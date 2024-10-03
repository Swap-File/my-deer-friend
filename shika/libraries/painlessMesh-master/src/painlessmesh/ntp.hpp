#ifndef _PAINLESS_MESH_NTP_HPP_
#define _PAINLESS_MESH_NTP_HPP_

#ifndef TIME_SYNC_INTERVAL
#define TIME_SYNC_INTERVAL 1 * TASK_MINUTE  // Time resync period
#endif

#ifndef TIME_SYNC_ACCURACY
#define TIME_SYNC_ACCURACY 5000  // Minimum time sync accuracy (5ms
#endif

#ifndef MIN_LARGE_OFFSET_STEP
#define MIN_LARGE_OFFSET_STEP 500  // Minimum offset size before softening
#endif

#ifndef MAX_CPU_TRIP_RATIO
#define MAX_CPU_TRIP_RATIO 85  // Maximum ratio between remote cpu time and trip time for NTP calc to be reasonable
#endif

#include "Arduino.h"

#include "painlessmesh/callback.hpp"
#include "painlessmesh/logger.hpp"
#include "painlessmesh/router.hpp"

extern painlessmesh::logger::LogClass Log;

namespace painlessmesh {
namespace ntp {

class MeshTime {
 public:
  /** Returns the mesh time in microsecond precision.
   *
   * Time rolls over every 71 minutes.
   *
   * Nodes try to keep a common time base synchronizing to each other using [an
   * SNTP based
   * protocol](https://gitlab.com/painlessMesh/painlessMesh/wikis/mesh-protocol#time-sync)
   */
  uint32_t getNodeTime() { return millis() + timeOffset; }

 protected:
  uint32_t timeOffset = 0;
};

/**
 * Calculate the offset of the local clock using the ntp algorithm
 *
 * See ntp overview for more information
 */
inline int32_t clockOffset(uint32_t time0, uint32_t time1, uint32_t time2,
                           uint32_t time3) {
  uint32_t offset =
      ((int32_t)(time1 - time0) / 2) + ((int32_t)(time2 - time3) / 2);

  // Take small steps to avoid over correction
  int32_t signedOffset = (int32_t)offset;
  if (abs(signedOffset) < MIN_LARGE_OFFSET_STEP && abs(signedOffset) > 4) signedOffset = signedOffset / 4;
  // Discard outliers with excessive trip to cpu ratios
  if ((time3-time0) > (time2-time1)*MAX_CPU_TRIP_RATIO) signedOffset = 0;
  return signedOffset;
}

/**
 * Calculate the time it took to get reply from other node
 *
 * See ntp algorithm for more information
 */
inline int32_t tripDelay(uint32_t time0, uint32_t time1, uint32_t time2,
                         uint32_t time3) {
  return ((time3 - time0) - (time2 - time1)) / 2;
}

inline bool adopt(protocol::NodeTree mesh, protocol::NodeTree connection) {
  auto mySubCount =
      layout::size(layout::excludeRoute(std::move(mesh), connection.nodeId));
  auto remoteSubCount = layout::size(connection);
  if (mySubCount > remoteSubCount) return false;
  if (mySubCount == remoteSubCount) {
    if (connection.nodeId == 0)
      Log(logger::ERROR, "Adopt called on uninitialized connection\n");
    // TODO: there is a change here that a middle node also lower is than the two others and will
    // start switching between both. Maybe should do it randomly instead?
    return mesh.nodeId < connection.nodeId;
  }
  return true;
}

template <class T>
void initTimeSync(protocol::NodeTree mesh, std::shared_ptr<T> connection,
                  uint32_t nodeTime) {
  using namespace painlessmesh::logger;
  painlessmesh::protocol::TimeSync timeSync;
  if (adopt(mesh, (*connection))) {
    timeSync = painlessmesh::protocol::TimeSync(mesh.nodeId, connection->nodeId,
                                                nodeTime);
    Log(S_TIME, "initTimeSync(): Requesting time from %u\n",
        connection->nodeId);
  } else {
    timeSync =
        painlessmesh::protocol::TimeSync(mesh.nodeId, connection->nodeId);
    Log(S_TIME, "initTimeSync(): Requesting %u to adopt our time\n",
        connection->nodeId);
  }

  router::send<protocol::TimeSync, T>(timeSync, connection, true);
}

template <class T, class U>
void handleTimeSync(T& mesh, painlessmesh::protocol::TimeSync timeSync,
                    std::shared_ptr<U> conn, uint32_t receivedAt) {
  switch (timeSync.msg.type) {
    case (painlessmesh::protocol::TIME_SYNC_ERROR):
      Log(logger::ERROR,
          "handleTimeSync(): Received time sync error. Restarting time "
          "sync.\n");
      conn->timeSyncTask.forceNextIteration();
      break;
    case (painlessmesh::protocol::TIME_SYNC_REQUEST):  // Other party request me
                                                       // to ask it for time
      Log(logger::S_TIME,
          "handleTimeSync(): Received requesto to start TimeSync with "
          "node: %u\n",
          conn->nodeId);
      timeSync.reply(mesh.getNodeTime());
      router::send<painlessmesh::protocol::TimeSync>(timeSync, conn, true);
      break;

    case (painlessmesh::protocol::TIME_REQUEST):
      timeSync.reply(receivedAt, mesh.getNodeTime());
      router::send<painlessmesh::protocol::TimeSync>(timeSync, conn, true);

      Log(logger::S_TIME,
          "handleTimeSync(): timeSyncStatus with %u completed\n", conn->nodeId);
		  
      if (mesh.nodeTimeSentCallback) {
        mesh.nodeTimeSentCallback( conn->nodeId);
      }
      conn->timeSyncTask.delay(TIME_SYNC_INTERVAL);
	        // After response is sent I assume sync is completed
      break;

    case (painlessmesh::protocol::TIME_REPLY): {
      Log(logger::S_TIME,
          "handleTimeSync(): %u adopting TIME_RESPONSE from %u\n", mesh.nodeId,
          conn->nodeId);
      int32_t offset = painlessmesh::ntp::clockOffset(
          timeSync.msg.t0, timeSync.msg.t1, timeSync.msg.t2, receivedAt);
      mesh.timeOffset += offset;  // Accumulate offset

      // flag all connections for re-timeSync
      if (mesh.nodeTimeAdjustedCallback) {
        mesh.nodeTimeAdjustedCallback(offset,conn->nodeId );
      }

      if ((offset < TIME_SYNC_ACCURACY) && (offset > -TIME_SYNC_ACCURACY) && (offset != 0)) {
        // mark complete only if offset was less than 10 ms
        conn->timeSyncTask.delay(TIME_SYNC_INTERVAL);
        Log(logger::S_TIME,
            "handleTimeSync(): timeSyncStatus with %u completed\n",
            conn->nodeId);

        // Time has changed, update other nodes
        for (auto&& connection : mesh.subs) {
          if (connection->nodeId != conn->nodeId) {  // exclude this connection
            connection->timeSyncTask.forceNextIteration();
            Log(logger::S_TIME,
                "handleTimeSync(): timeSyncStatus with %u brought forward\n",
                connection->nodeId);
          }
        }
      } else {
        // Iterate sync procedure if accuracy was not enough
        conn->timeSyncTask.delay(200 * TASK_MILLISECOND);  // Small delay
        Log(logger::S_TIME,
            "handleTimeSync(): timeSyncStatus with %u needs further tries\n",
            conn->nodeId);
      }
      break;
    }
    default:
      Log(logger::ERROR, "handleTimeSync(): unkown type %u, %u\n",
          timeSync.msg.type, painlessmesh::protocol::TIME_SYNC_REQUEST);
      break;
  }
  Log(logger::S_TIME, "handleTimeSync(): ----------------------------------\n");
}

template <class T, class U>
void handleTimeDelay(T& mesh, painlessmesh::protocol::TimeDelay timeDelay,
                     std::shared_ptr<U> conn, uint32_t receivedAt) {
  Log(logger::S_TIME, "handleTimeDelay(): from %u in timestamp\n",
      timeDelay.from);

  switch (timeDelay.msg.type) {
    case (painlessmesh::protocol::TIME_SYNC_ERROR):
      Log(logger::ERROR,
          "handleTimeDelay(): Error in requesting time delay. Please try "
          "again.\n");
      break;

    case (painlessmesh::protocol::TIME_REQUEST):
      // conn->timeSyncStatus == IN_PROGRESS;
      Log(logger::S_TIME, "handleTimeDelay(): TIME REQUEST received.\n");

      // Build time response
      timeDelay.reply(receivedAt, mesh.getNodeTime());
      router::send<protocol::TimeDelay, U>(timeDelay, conn);
      break;

    case (painlessmesh::protocol::TIME_REPLY): {
      Log(logger::S_TIME, "handleTimeDelay(): TIME RESPONSE received.\n");
      int32_t delay = painlessmesh::ntp::tripDelay(
          timeDelay.msg.t0, timeDelay.msg.t1, timeDelay.msg.t2, receivedAt);
      Log(logger::S_TIME, "handleTimeDelay(): Delay is %d\n", delay);

      // conn->timeSyncStatus == COMPLETE;

      if (mesh.nodeDelayReceivedCallback)
        mesh.nodeDelayReceivedCallback(timeDelay.from, delay);
    } break;

    default:
      Log(logger::ERROR,
          "handleTimeDelay(): Unknown timeSyncMessageType received. Ignoring "
          "for now.\n");
  }

  Log(logger::S_TIME, "handleTimeSync(): ----------------------------------\n");
}

template <class T, typename U>
callback::MeshPackageCallbackList<U> addPackageCallback(
    callback::MeshPackageCallbackList<U>&& callbackList, T& mesh) {
  // TimeSync
  callbackList.onPackage(
      protocol::TIME_SYNC,
      [&mesh](protocol::Variant variant, std::shared_ptr<U> connection,
              uint32_t receivedAt) {
        auto timeSync = variant.to<protocol::TimeSync>();
        handleTimeSync<T, U>(mesh, timeSync, connection, receivedAt);
        return false;
      });

  // TimeDelay
  callbackList.onPackage(
      protocol::TIME_DELAY,
      [&mesh](protocol::Variant variant, std::shared_ptr<U> connection,
              uint32_t receivedAt) {
        auto timeDelay = variant.to<protocol::TimeDelay>();
        handleTimeDelay<T, U>(mesh, timeDelay, connection, receivedAt);
        return false;
      });

  return callbackList;
}

}  // namespace ntp
}  // namespace painlessmesh
#endif
