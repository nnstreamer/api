/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2022 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file service-db.hh
 * @date 28 Mar 2022
 * @brief NNStreamer/Service Database Interface
 * @see	https://github.com/nnstreamer/api
 * @author Sangjung Woo <sangjung.woo@samsung.com>
 * @bug No known bugs except for NYI items
 */

#ifndef __SERVICE_DB_HH__
#define __SERVICE_DB_HH__

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
  virtual void disconnectDB () = 0;
  virtual void put (const std::string key, const std::string value) = 0;
  virtual void get (const std::string name,
      std::string & out_value) = 0;
  virtual void del (const std::string name) = 0;
};

/**
 * @brief Class for implementation of IMLServiceDB
 */
class MLServiceLevelDB : public IMLServiceDB
{
public:
  MLServiceLevelDB (const MLServiceLevelDB &) = delete;
  MLServiceLevelDB (MLServiceLevelDB &&) = delete;
  MLServiceLevelDB & operator= (const MLServiceLevelDB &) = delete;
  MLServiceLevelDB & operator= (MLServiceLevelDB &&) = delete;

  virtual void connectDB () override;
  virtual void disconnectDB () override;
  virtual void put (const std::string name,
      const std::string value) override;
  virtual void get (std::string name,
      std::string & out_value) override;
  virtual void del (std::string name);

  static IMLServiceDB & getInstance (void);

private:
  MLServiceLevelDB (std::string path);
  virtual ~MLServiceLevelDB ();

  std::string path;
  leveldb_t *db_obj;
  leveldb_readoptions_t *db_roptions;
  leveldb_writeoptions_t *db_woptions;
};

#endif /* __SERVICE_DB_HH__ */
