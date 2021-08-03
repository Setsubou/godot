/*************************************************************************/
/*  debug_adapter_protocol.h                                             */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#ifndef DEBUG_ADAPTER_PROTOCOL_H
#define DEBUG_ADAPTER_PROTOCOL_H

#include "core/io/stream_peer.h"
#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"

#include "debug_adapter_parser.h"
#include "debug_adapter_types.h"

#define DAP_MAX_BUFFER_SIZE 4194304 // 4MB
#define DAP_MAX_CLIENTS 8

class DebugAdapterParser;

struct DAPeer : RefCounted {
	Ref<StreamPeerTCP> connection;

	uint8_t req_buf[DAP_MAX_BUFFER_SIZE];
	int req_pos = 0;
	bool has_header = false;
	int content_length = 0;
	List<Dictionary> res_queue;
	int seq = 0;

	// Client specific info
	bool linesStartAt1 = false;
	bool columnsStartAt1 = false;
	bool supportsVariableType = false;
	bool supportsInvalidatedEvent = false;

	Error handle_data();
	Error send_data();
	String format_output(const Dictionary &p_params) const;
};

class DebugAdapterProtocol : public Object {
	GDCLASS(DebugAdapterProtocol, Object)

	friend class DebugAdapterParser;

private:
	static DebugAdapterProtocol *singleton;
	DebugAdapterParser *parser;

	List<Ref<DAPeer>> clients;
	Ref<TCPServer> server;

	Error on_client_connected();
	void on_client_disconnected(const Ref<DAPeer> &p_peer);
	void on_debug_paused();
	void on_debug_stopped();
	void on_debug_output(const String &p_message);
	void on_debug_breaked(const bool &p_reallydid, const bool &p_can_debug, const String &p_reason, const bool &p_has_stackdump);
	void on_debug_stack_dump(const Array &p_stack_dump);
	void on_debug_stack_frame_vars(const int &p_size);
	void on_debug_stack_frame_var(const Array &p_data);

	void reset_current_info();
	void reset_ids();
	void reset_stack_info();

	bool _initialized = false;
	bool _processing_breakpoint = false;
	bool _stepping = false;
	bool _processing_stackdump = false;
	int _remaining_vars = 0;
	int _current_frame = 0;

	String _current_request;
	Ref<DAPeer> _current_peer;

	int breakpoint_id;
	int stackframe_id;
	int variable_id;
	List<DAP::Breakpoint> breakpoint_list;
	Map<DAP::StackFrame, List<int>> stackframe_list;
	Map<int, Array> variable_list;

public:
	_FORCE_INLINE_ static DebugAdapterProtocol *get_singleton() { return singleton; }
	_FORCE_INLINE_ bool is_active() const { return _initialized && clients.size() > 0; }

	bool process_message(const String &p_text);

	String get_current_request() const { return _current_request; }
	Ref<DAPeer> get_current_peer() const { return _current_peer; }

	void notify_initialized();
	void notify_process();
	void notify_terminated();
	void notify_exited(const int &p_exitcode = 0);
	void notify_stopped_paused();
	void notify_stopped_exception(const String &p_error);
	void notify_stopped_breakpoint(const int &p_id);
	void notify_stopped_step();
	void notify_continued();
	void notify_output(const String &p_message);

	Array update_breakpoints(const String &p_path, const Array &p_breakpoints);

	void poll();
	Error start(int p_port, const IPAddress &p_bind_ip);
	void stop();

	DebugAdapterProtocol();
	~DebugAdapterProtocol();
};

#endif
