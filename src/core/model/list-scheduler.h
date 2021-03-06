/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef LIST_SCHEDULER_H
#define LIST_SCHEDULER_H

#include "scheduler.h"
#include <list>
#include <utility>
#include <stdint.h>

/**
 * \file
 * \ingroup scheduler
 * Declaration of ns3::ListScheduler class.
 */

namespace ns3 {

class EventImpl;

/**
 * \ingroup scheduler
 * \brief a std::list event scheduler
 *
 * This class implements an event scheduler using an std::list
 * data structure, that is, a double linked-list.
 */
class ListScheduler : public Scheduler
{
public:
  static TypeId GetTypeId (void);

  ListScheduler ();
  virtual ~ListScheduler ();

  virtual void Insert (const Event &ev);
  virtual bool IsEmpty (void) const;
  virtual Event PeekNext (void) const;
  virtual Event RemoveNext (void);
  virtual void Remove (const Event &ev);

private:
  typedef std::list<Event> Events;
  typedef std::list<Event>::iterator EventsI;
  Events m_events;
};

} // namespace ns3

#endif /* LIST_SCHEDULER_H */
