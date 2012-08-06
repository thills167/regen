/*
 * event-object.cpp
 *
 *  Created on: 29.01.2011
 *      Author: daniel
 */

#include "event-object.h"

set< pair<EventObject*, unsigned int> > EventObject::queued_ =
    set< pair<EventObject*, unsigned int> >();
boost::mutex EventObject::eventLock_;

unsigned int EventCallable::id() const
{
  return id_;
}
void EventCallable::set_id(unsigned int id)
{
  id_ = id;
}

//// static

unsigned int &EventObject::numEvents()
{
  static unsigned int numEvents_ = 0;
  return numEvents_;
}

map<string,unsigned int> &EventObject::eventIds()
{
  static map<string,unsigned int> eventIds_ = map<string,unsigned int>();
  return eventIds_;
}

///// none static

EventObject::EventObject()
: handlerCounter_(0),
  eventHandlers_()
{
}

unsigned int EventObject::registerEvent(const string &eventName)
{
  ++EventObject::numEvents();
  EventObject::eventIds().insert( pair<string,unsigned int>(
      eventName, EventObject::numEvents()));
  return EventObject::numEvents();
}

unsigned int EventObject::connect(unsigned int eventId, ref_ptr<EventCallable> callable)
{
  EventHandlers::iterator it = eventHandlers_.find(eventId);

  ++handlerCounter_;
  eventHandlerIds_[handlerCounter_] = eventId;

  EventData p(callable, handlerCounter_);
  if( it != eventHandlers_.end() ) {
    it->second.push_back(p);
  } else {
    EventHandlerList newList;
    newList.push_back( p );
    eventHandlers_[eventId] = newList;
  }

  callable->set_id(handlerCounter_);

  return handlerCounter_;
}
unsigned int EventObject::connect(const string &eventName, ref_ptr<EventCallable> callable)
{
  return connect(EventObject::eventIds()[eventName], callable);
}

void EventObject::disconnect(unsigned int connectionID)
{
  EventHandlerIds::iterator idNeedle =
      eventHandlerIds_.find(connectionID);
  if(idNeedle == eventHandlerIds_.end()) return; // handler id not found!

  unsigned int eventId = idNeedle->second;
  EventHandlers::iterator signalHandlers =
      eventHandlers_.find(eventId);
  if(signalHandlers == eventHandlers_.end()) return; // no handlers not found!

  EventHandlerList &l = signalHandlers->second;
  for(EventHandlerList::iterator it = l.begin(); it != l.end(); ++it)
  {
    if(it->second != connectionID) continue;

    l.erase(it);
    break;
  }

  eventHandlerIds_.erase(idNeedle);
}
void EventObject::disconnect(ref_ptr<EventCallable> c)
{
  disconnect(c->id());
}

void EventObject::emit(unsigned int eventID, void *data)
{
  EventHandlers::iterator it = eventHandlers_.find(eventID);
  if( it != eventHandlers_.end() ) {
    EventHandlerList::iterator jt;
    for(jt = it->second.begin(); jt != it->second.end(); ++jt)
    {
      jt->first->call(this, data);
    }
  }
}
void EventObject::emit(const string &eventName, void *data)
{
  return emit(EventObject::eventIds()[eventName], data);
}

void EventObject::emitQueued()
{
  set< pair<EventObject*, unsigned int> > events;
  // get a lock an receive list with queued signals
  eventLock_.lock(); {
    std::copy( queued_.begin(), queued_.end(),
        std::insert_iterator<std::set< pair<EventObject*, unsigned int> > >( events, events.begin() ) );
    queued_.clear();
  } eventLock_.unlock();
  // and emit the queued signals
  for(set< pair<EventObject*, unsigned int> >::iterator it = events.begin(); it != events.end(); ++it)
  {
    it->first->emit(it->second);
  }
}

void EventObject::queueEmit(unsigned int eventID)
{
  // get a lock and push the event into set of queued signals
  eventLock_.lock(); {
    queued_.insert(pair<EventObject*, unsigned int>(this,eventID));
  } eventLock_.unlock();
}
void EventObject::queueEmit(const string &eventName)
{
  return queueEmit(EventObject::eventIds()[eventName]);
}
