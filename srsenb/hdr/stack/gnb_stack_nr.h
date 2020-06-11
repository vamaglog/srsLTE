/*
 * Copyright 2013-2020 Software Radio Systems Limited
 *
 * This file is part of srsLTE.
 *
 * srsLTE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsLTE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/******************************************************************************
 * File:        gnb_stack_nr.h
 * Description: L2/L3 gNB stack class.
 *****************************************************************************/

#ifndef SRSLTE_GNB_STACK_NR_H
#define SRSLTE_GNB_STACK_NR_H

#include "srsenb/hdr/stack/mac/mac_nr.h"
#include "srsenb/hdr/stack/rrc/rrc_nr.h"
#include "srsenb/hdr/stack/upper/pdcp_nr.h"
#include "srsenb/hdr/stack/upper/rlc_nr.h"
#include "upper/gtpu.h"
#include "upper/s1ap.h"
#include "upper/sdap.h"

#include "srslte/common/log_filter.h"

#include "enb_stack_base.h"
#include "srsenb/hdr/enb.h"
#include "srslte/interfaces/gnb_interfaces.h"

// This is needed for GW
#include "srslte/interfaces/ue_interfaces.h"
#include "srsue/hdr/stack/upper/gw.h"

namespace srsenb {

class gnb_stack_nr final : public srsenb::enb_stack_base,
                           public stack_interface_phy_nr,
                           public stack_interface_mac,
                           public srsue::stack_interface_gw,
                           public srslte::task_handler_interface,
                           public srslte::thread
{
public:
  explicit gnb_stack_nr(srslte::logger* logger_);
  ~gnb_stack_nr() final;

  int init(const srsenb::stack_args_t& args_, const rrc_nr_cfg_t& rrc_cfg_, phy_interface_stack_nr* phy_);
  int init(const srsenb::stack_args_t& args_, const rrc_nr_cfg_t& rrc_cfg_);

  // eNB stack base interface
  void        stop() final;
  std::string get_type() final;
  bool        get_metrics(srsenb::stack_metrics_t* metrics) final;

  // PHY->MAC interface
  int sf_indication(const uint32_t tti);
  int rx_data_indication(rx_data_ind_t& grant);

  // Temporary GW interface
  void write_sdu(uint32_t lcid, srslte::unique_byte_buffer_t sdu, bool blocking);
  bool is_lcid_enabled(uint32_t lcid);
  bool switch_on();
  void run_tti(uint32_t tti);

  // MAC interface to trigger processing of received PDUs
  void process_pdus() final;

  // Task Handling interface
  srslte::timer_handler::unique_timer    get_unique_timer() final { return timers.get_unique_timer(); }
  srslte::task_multiqueue::queue_handler make_task_queue() final { return pending_tasks.get_queue_handler(); }
  void                                   enqueue_background_task(std::function<void(uint32_t)> f) final;
  void                                   notify_background_task_result(srslte::move_task_t task) final;
  void                                   defer_callback(uint32_t duration_ms, std::function<void()> func) final;
  void                                   defer_task(srslte::move_task_t task) final;

private:
  void run_thread() final;
  void run_tti_impl(uint32_t tti);

  // args
  srsenb::stack_args_t    args   = {};
  srslte::logger*         logger = nullptr;
  phy_interface_stack_nr* phy    = nullptr;

  /* Functions for MAC Timers */
  srslte::timer_handler timers;

  // derived
  std::unique_ptr<mac_nr>    m_mac;
  std::unique_ptr<rlc_nr>    m_rlc;
  std::unique_ptr<pdcp_nr>   m_pdcp;
  std::unique_ptr<sdap>      m_sdap;
  std::unique_ptr<rrc_nr>    m_rrc;
  std::unique_ptr<srsue::gw> m_gw;
  //  std::unique_ptr<ngap>      m_ngap;
  //  std::unique_ptr<srsenb::gtpu> m_gtpu;

  // state
  bool     running     = false;
  uint32_t current_tti = 10240;

  // Thread
  static const int                 STACK_MAIN_THREAD_PRIO = -1; // Use default high-priority below UHD
  srslte::task_multiqueue          pending_tasks;
  std::vector<srslte::move_task_t> deferred_stack_tasks; ///< enqueues stack tasks from within. Avoids locking
  srslte::task_thread_pool         background_tasks;     ///< Thread pool used for long, low-priority tasks
  int sync_queue_id = -1, ue_queue_id = -1, gw_queue_id = -1, mac_queue_id = -1, background_queue_id = -1;
};

} // namespace srsenb

#endif // SRSLTE_GNB_STACK_NR_H
