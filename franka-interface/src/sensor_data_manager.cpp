#include "franka-interface/sensor_data_manager.h"

#include <iostream>

#include <google/protobuf/message.h>

SensorDataManagerReadStatus SensorDataManager::readSensorMessage(google::protobuf::Message &message) {
   std::function<bool(const void *bytes, int data_size)>
       parse_callback = [&](const void *bytes, int data_size) -> bool {
     // get state variables
     return message.ParseFromArray(bytes, data_size);
   };
   return readMessageAsBytes(parse_callback);
}

SensorDataManagerReadStatus SensorDataManager::readMessageAsBytes(std::function< bool(const void *bytes, int data_size)> parse_callback) {
  SensorDataManagerReadStatus status;

  try {
    if (buffer_mutex_->try_lock()) {
      int has_new_message = static_cast<int>(buffer_[0]);
      if (has_new_message == 1) {
        int sensor_msg_type = static_cast<int>(buffer_[1]);
        int data_size = (buffer_[2] + (buffer_[3] << 8) + (buffer_[4] << 16) + (buffer_[5] << 24));

        if (parse_callback(buffer_ + 6, data_size)) {
          status = SensorDataManagerReadStatus::SUCCESS;
          buffer_[0] = 0;
        } else {
          status = SensorDataManagerReadStatus::FAIL_TO_READ;
        }
      } else {
        status = SensorDataManagerReadStatus::NO_NEW_MESSAGE;
      }
      buffer_mutex_->unlock();
    }
  } catch (boost::interprocess::lock_exception) {
    status = SensorDataManagerReadStatus::FAIL_TO_GET_LOCK;
  }

  return status;

}

void SensorDataManager::clearBuffer() {
  try {
    if (buffer_mutex_->try_lock()) {
      buffer_[0] = 0;
      buffer_mutex_->unlock();
    }
  } catch (boost::interprocess::lock_exception) {
  }
}