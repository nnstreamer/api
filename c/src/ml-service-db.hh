/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-service-db.hh
 * @date 28 Mar 2022
 * @brief NNStreamer/Service Database Interface
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#pragma once

#include <leveldb/c.h>
#include <iostream>

/**
 * @brief Interface for database operation of ML service
 */
class IMLServiceDB
{
public:
  /**
   * @brief Destroy the IMLServiceDB object
   */
  virtual ~IMLServiceDB ()
  {
  };
  virtual void connectDB () = 0;
  virtual void setPipelineDescription (const std::string name,
      const std::string pipeline_description) = 0;
  virtual void getPipelineDescription (const std::string name,
      std::string & pipeline_description) = 0;
  virtual void delPipelineDescription (const std::string name) = 0;
};
