// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/animation/animation_container.h"

#include "ui/gfx/animation/animation_container_element.h"
#include "ui/gfx/animation/animation_container_observer.h"

using base::TimeDelta;
using base::TimeTicks;

namespace gfx {

AnimationContainer::AnimationContainer()
    : last_tick_time_(base::TimeTicks::Now()),
      min_timer_interval_count_(0),
      observer_(NULL) {}

AnimationContainer::~AnimationContainer() {
  // The animations own us and stop themselves before being deleted. If
  // elements_ is not empty, something is wrong.
  DCHECK(elements_.empty());
}

void AnimationContainer::Start(AnimationContainerElement* element) {
  DCHECK(elements_.count(element) == 0);  // Start should only be invoked if the
                                          // element isn't running.

  if (elements_.empty()) {
    last_tick_time_ = base::TimeTicks::Now();
    SetMinTimerInterval(element->GetTimerInterval());
    min_timer_interval_count_ = 1;
  } else if (element->GetTimerInterval() < min_timer_interval_) {
    SetMinTimerInterval(element->GetTimerInterval());
    min_timer_interval_count_ = 1;
  } else if (element->GetTimerInterval() == min_timer_interval_) {
    min_timer_interval_count_++;
  }

  element->SetStartTime(last_tick_time_);
  elements_.insert(element);
}

void AnimationContainer::Stop(AnimationContainerElement* element) {
  DCHECK(elements_.count(element) > 0);  // The element must be running.

  base::TimeDelta interval = element->GetTimerInterval();
  elements_.erase(element);

  if (elements_.empty()) {
    timer_.Stop();
    min_timer_interval_count_ = 0;
    if (observer_)
      observer_->AnimationContainerEmpty(this);
  } else if (interval == min_timer_interval_) {
    min_timer_interval_count_--;

    // If the last element at the current (minimum) timer interval has been
    // removed then go find the new minimum and the number of elements at that
    // same minimum.
    if (min_timer_interval_count_ == 0) {
      std::pair<base::TimeDelta, size_t> interval_count =
          GetMinIntervalAndCount();
      DCHECK(interval_count.first > min_timer_interval_);
      SetMinTimerInterval(interval_count.first);
      min_timer_interval_count_ = interval_count.second;
    }
  }
}

void AnimationContainer::Run() {
  // We notify the observer after updating all the elements. If all the elements
  // are deleted as a result of updating then our ref count would go to zero and
  // we would be deleted before we notify our observer. We add a reference to
  // ourself here to make sure we're still valid after running all the elements.
  scoped_refptr<AnimationContainer> this_ref(this);

  TimeTicks current_time = base::TimeTicks::Now();

  last_tick_time_ = current_time;

  // Make a copy of the elements to iterate over so that if any elements are
  // removed as part of invoking Step there aren't any problems.
  Elements elements = elements_;

  for (Elements::const_iterator i = elements.begin();
       i != elements.end(); ++i) {
    // Make sure the element is still valid.
    if (elements_.find(*i) != elements_.end())
      (*i)->Step(current_time);
  }

  if (observer_)
    observer_->AnimationContainerProgressed(this);
}

void AnimationContainer::SetMinTimerInterval(base::TimeDelta delta) {
  // This doesn't take into account how far along the current element is, but
  // that shouldn't be a problem for uses of Animation/AnimationContainer.
  timer_.Stop();
  min_timer_interval_ = delta;
  timer_.Start(FROM_HERE, min_timer_interval_, this, &AnimationContainer::Run);
}

std::pair<TimeDelta, size_t> AnimationContainer::GetMinIntervalAndCount()
    const {
  DCHECK(!elements_.empty());

  // Find the minimum interval and the number of elements sharing that same
  // interval. It is tempting to create a map of intervals -> counts in order to
  // make this O(log n) instead of O(n). However, profiling shows that this
  // offers no practical performance gain (the most common case is that all
  // elements in the set share the same interval).
  TimeDelta min;
  size_t count = 1;
  auto i = elements_.begin();
  min = (*i)->GetTimerInterval();
  for (++i; i != elements_.end(); ++i) {
    auto interval = (*i)->GetTimerInterval();
    if (interval < min) {
      min = interval;
      count = 1;
    } else if (interval == min) {
      count++;
    }
  }

  return std::make_pair(min, count);
}

}  // namespace gfx
