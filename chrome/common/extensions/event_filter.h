// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_EVENT_FILTER_H_
#define CHROME_COMMON_EXTENSIONS_EVENT_FILTER_H_
#pragma once

#include "base/memory/linked_ptr.h"
#include "chrome/common/extensions/event_matcher.h"
#include "chrome/common/extensions/event_filtering_info.h"
#include "chrome/common/extensions/matcher/url_matcher.h"

#include <map>
#include <set>

namespace extensions {

// Matches incoming events against a collection of EventMatchers. Each added
// EventMatcher is given an id which is returned by MatchEvent() when it is
// passed a matching event.
class EventFilter {
 public:
  typedef int MatcherID;
  EventFilter();
  ~EventFilter();

  // Adds an event matcher that will be used in calls to MatchEvent(). Returns
  // the id of the matcher, or -1 if there was an error.
  MatcherID AddEventMatcher(const std::string& event_name,
                            scoped_ptr<EventMatcher> matcher);
  // Removes an event matcher, returning the name of the event that it was for.
  std::string RemoveEventMatcher(MatcherID id);

  // Match an event named |event_name| with filtering info |event_info| against
  // our set of event matchers. Returns a set of ids that correspond to the
  // event matchers that matched the event.
  // TODO(koz): Add a std::string* parameter for retrieving error messages.
  std::set<MatcherID> MatchEvent(const std::string& event_name,
                                 const EventFilteringInfo& event_info);

  int GetMatcherCountForEvent(const std::string& event_name);

  // For testing.
  bool IsURLMatcherEmpty() const {
    return url_matcher_.IsEmpty();
  }

 private:
  class EventMatcherEntry {
   public:
    // Adds |condition_sets| to |url_matcher| on construction and removes them
    // again on destruction. |condition_sets| should be the
    // URLMatcherConditionSets that match the URL constraints specified by
    // |event_matcher|.
    EventMatcherEntry(scoped_ptr<EventMatcher> event_matcher,
                      URLMatcher* url_matcher,
                      const URLMatcherConditionSet::Vector& condition_sets);
    ~EventMatcherEntry();

    // Prevents the removal of condition sets when this class is destroyed. We
    // call this in EventFilter's destructor so that we don't do the costly
    // removal of condition sets when the URLMatcher is going to be destroyed
    // and clean them up anyway.
    void DontRemoveConditionSetsInDestructor();

    const EventMatcher& event_matcher() const {
      return *event_matcher_;
    }

   private:
    scoped_ptr<EventMatcher> event_matcher_;
    // The id sets in url_matcher_ that this EventMatcher owns.
    std::vector<URLMatcherConditionSet::ID> condition_set_ids_;
    URLMatcher* url_matcher_;

    DISALLOW_COPY_AND_ASSIGN(EventMatcherEntry);
  };

  // Maps from a matcher id to an event matcher entry.
  typedef std::map<MatcherID, linked_ptr<EventMatcherEntry> > EventMatcherMap;

  // Maps from event name to the map of matchers that are registered for it.
  typedef std::map<std::string, EventMatcherMap> EventMatcherMultiMap;

  // Adds the list of filters to the URL matcher, having matches for those URLs
  // map to |id|.
  bool CreateConditionSets(MatcherID id,
                           base::ListValue* url_filters,
                           URLMatcherConditionSet::Vector* condition_sets);

  bool AddDictionaryAsConditionSet(
      base::DictionaryValue* url_filter,
      URLMatcherConditionSet::Vector* condition_sets);

  URLMatcher url_matcher_;
  EventMatcherMultiMap event_matchers_;

  // The next id to assign to an EventMatcher.
  MatcherID next_id_;

  // The next id to assign to a condition set passed to URLMatcher.
  URLMatcherConditionSet::ID next_condition_set_id_;

  // Maps condition set ids, which URLMatcher operates in, to event matcher
  // ids, which the interface to this class operates in. As each EventFilter
  // can specify many condition sets this is a many to one relationship.
  std::map<URLMatcherConditionSet::ID, MatcherID>
      condition_set_id_to_event_matcher_id_;

  // Maps from event matcher ids to the name of the event they match on.
  std::map<MatcherID, std::string> id_to_event_name_;

  DISALLOW_COPY_AND_ASSIGN(EventFilter);
};

}  // namespace extensions

#endif  // CHROME_COMMON_EXTENSIONS_EVENT_FILTER_H_
