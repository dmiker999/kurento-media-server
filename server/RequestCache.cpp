/*
 * (C) Copyright 2014 Kurento (http://kurento.org/)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "RequestCache.hpp"
#include <gst/gst.h>

#include "CacheEntry.hpp"

#define GST_CAT_DEFAULT kurento_request_cache
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "KurentoRequestCache"

namespace kurento
{

RequestCache::RequestCache (unsigned int timeout)
{
  this->timeout = timeout;
}

RequestCache::~RequestCache ()
{
}

void
RequestCache::addResponse (std::string sessionId, std::string requestId,
                           Json::Value &response)
{
  std::shared_ptr<CacheEntry> entry;
  std::unique_lock<std::recursive_mutex> lock (mutex);

  entry = std::shared_ptr<CacheEntry> (new CacheEntry (timeout, sessionId,
                                       requestId, response) );

  cache[sessionId][requestId] = entry;
  entry->signalTimeout.connect ([this, sessionId, requestId] () {
    std::unique_lock<std::recursive_mutex> lock (mutex);
    auto it1 = this->cache.find (sessionId);

    if (it1 == this->cache.end() ) {
      return;
    }

    auto it2 = this->cache[sessionId].find (requestId);

    if (it2 == this->cache[sessionId].end() ) {
      return;
    }

    this->cache[sessionId].erase (it2);

    if (this->cache[sessionId].empty() ) {
      this->cache.erase (it1);
    }
  });
}

Json::Value
RequestCache::getCachedResponse (std::string sessionId, std::string requestId)
{
  std::map<std::string, std::shared_ptr <CacheEntry>> requests;

  std::unique_lock<std::recursive_mutex> lock (mutex);

  auto it1 = cache.find (sessionId);

  if (it1 == cache.end() ) {
    throw CacheException ("Session not cached");
  }

  requests = cache[sessionId];
  auto it2 = requests.find (requestId);

  if (it2 == requests.end() ) {
    throw CacheException ("Response not cached");
  }

  return requests[requestId]->getResponse();
}

RequestCache::StaticConstructor RequestCache::staticConstructor;

RequestCache::StaticConstructor::StaticConstructor()
{
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
                           GST_DEFAULT_NAME);
}

} // kurento
