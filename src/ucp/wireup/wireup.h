/**
 * Copyright (C) Mellanox Technologies Ltd. 2001-2015.  ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#ifndef UCP_WIREUP_H_
#define UCP_WIREUP_H_

#include <ucp/api/ucp.h>
#include <ucp/core/ucp_context.h>
#include <ucp/core/ucp_ep.h>
#include <ucp/core/ucp_worker.h>
#include <uct/api/uct.h>
#include <ucs/arch/bitops.h>


/**
 * Wireup message types
 */
enum {
    UCP_WIREUP_MSG_PRE_REQUEST,
    UCP_WIREUP_MSG_REQUEST,
    UCP_WIREUP_MSG_REPLY,
    UCP_WIREUP_MSG_ACK,
    UCP_WIREUP_MSG_LAST
};


/**
 * Criteria for transport selection.
 */
typedef struct {
    const char  *title;            /* Name of the criteria for debugging */
    uint64_t    local_md_flags;    /* Required local MD flags */
    uint64_t    remote_md_flags;   /* Required remote MD flags */
    uint64_t    local_iface_flags; /* Required local interface flags */
    uint64_t    remote_iface_flags;/* Required remote interface flags */

    /**
     * Calculates score of a potential transport.
     *
     * @param [in]  context      UCP context.
     * @param [in]  md_attr      Local MD attributes.
     * @param [in]  iface_attr   Local interface attributes.
     * @param [in]  remote_info  Remote peer attributes.
     *
     * @return Transport score, the higher the better.
     */
    double      (*calc_score)(ucp_context_h context,
                              const uct_md_attr_t *md_attr,
                              const uct_iface_attr_t *iface_attr,
                              const ucp_address_iface_attr_t *remote_iface_attr);
    uint8_t     tl_rsc_flags; /* Flags that describe TL specifics */

    ucp_tl_iface_atomic_flags_t local_atomic_flags;
    ucp_tl_iface_atomic_flags_t remote_atomic_flags;
} ucp_wireup_criteria_t;


/**
 * Packet structure for wireup requests.
 */
typedef struct ucp_wireup_msg {
    uint8_t                 type;         /* Message type */
    ucp_err_handling_mode_t err_mode;     /* Peer error handling mode */
    ucp_ep_conn_sn_t        conn_sn;      /* Connection sequence number */
    uintptr_t               src_ep_ptr;   /* Endpoint of source */
    uintptr_t               dest_ep_ptr;  /* Endpoint of destination (0 - invalid) */

    /* REQUEST - which p2p lanes must be connected
     * REPLY - which p2p lanes have been connected
     */
    uint8_t                 tli[UCP_MAX_LANES];

    /* packed addresses follow */
} UCS_S_PACKED ucp_wireup_msg_t;


ucs_status_t ucp_wireup_send_request(ucp_ep_h ep);

ucs_status_t ucp_wireup_send_pre_request(ucp_ep_h ep);

ucs_status_t ucp_wireup_connect_remote(ucp_ep_h ep, ucp_lane_index_t lane);

ucs_status_t ucp_wireup_select_aux_transport(ucp_ep_h ep,
                                             const ucp_ep_params_t *params,
                                             const ucp_address_entry_t *address_list,
                                             unsigned address_count,
                                             ucp_rsc_index_t *rsc_index_p,
                                             unsigned *addr_index_p);

ucs_status_t ucp_wireup_select_sockaddr_transport(ucp_ep_h ep,
                                                  const ucp_ep_params_t *params,
                                                  ucp_rsc_index_t *rsc_index_p);

double ucp_wireup_amo_score_func(ucp_context_h context,
                                 const uct_md_attr_t *md_attr,
                                 const uct_iface_attr_t *iface_attr,
                                 const ucp_address_iface_attr_t *remote_iface_attr);

ucs_status_t ucp_wireup_msg_progress(uct_pending_req_t *self);

int ucp_wireup_msg_ack_cb_pred(const ucs_callbackq_elem_t *elem, void *arg);

ucs_status_t ucp_wireup_init_lanes(ucp_ep_h ep, const ucp_ep_params_t *params,
                                   unsigned ep_init_flags, unsigned address_count,
                                   const ucp_address_entry_t *address_list,
                                   uint8_t *addr_indices);

ucs_status_t ucp_wireup_select_lanes(ucp_ep_h ep, const ucp_ep_params_t *params,
                                     unsigned ep_init_flags, unsigned address_count,
                                     const ucp_address_entry_t *address_list,
                                     uint8_t *addr_indices,
                                     ucp_ep_config_key_t *key);

ucs_status_t ucp_signaling_ep_create(ucp_ep_h ucp_ep, uct_ep_h uct_ep,
                                     int is_owner, uct_ep_h *signaling_ep);

static inline int ucp_worker_is_tl_p2p(ucp_worker_h worker, ucp_rsc_index_t rsc_index)
{
    uint64_t flags = ucp_worker_iface_get_attr(worker, rsc_index)->cap.flags;

    return (flags & UCT_IFACE_FLAG_CONNECT_TO_EP) &&
           !(flags & UCT_IFACE_FLAG_CONNECT_TO_IFACE);
}

#endif
