#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "../hspsdk/hsp3debug.h"
#include "../knowbug_core/platform.h"

class HspObjects;
class HspObjectListDelta;
class KnowbugMessage;
class KnowbugReceiver;
class KnowbugSender;

// クライアントへのコネクションを表す
// (クライアントを起動・停止すること、クライアントへメッセージを送る／クライアントから受信することを役割とし、それ以外のことはしない)
class KnowbugServer {
public:
	static auto create(HSP3DEBUG* debug, HspObjects& objects, HINSTANCE instance, KnowbugReceiver& receiver) -> std::shared_ptr<KnowbugServer>;

	virtual ~KnowbugServer() {
	}

	virtual auto get_sender() -> KnowbugSender & = 0;

	virtual void start() = 0;

	virtual void will_exit() = 0;

	virtual void logmes(HspStringView text) = 0;

	virtual void debuggee_did_stop() = 0;
};

// クライアントからのメッセージを処理するメソッドに渡されるパラメーター
class IncomingCtx {
public:
	const KnowbugServer& server;
	KnowbugSender& sender;
	KnowbugMessage& msg;
};

// クライアントからのメッセージを受信するインターフェイス
class KnowbugReceiver {
public:
	virtual ~KnowbugReceiver() = 0;

	//virtual void client_did_send_something(IncomingCtx ctx) = 0;
	virtual void client_did_initialize(IncomingCtx ctx) = 0;
	virtual void client_did_terminate(IncomingCtx ctx) = 0;
	virtual void client_did_step_continue(IncomingCtx ctx) = 0;
	virtual void client_did_step_pause(IncomingCtx ctx) = 0;
	virtual void client_did_step_in(IncomingCtx ctx) = 0;
	virtual void client_did_step_over(IncomingCtx ctx) = 0;
	virtual void client_did_step_out(IncomingCtx ctx) = 0;
	virtual void client_did_location_update(IncomingCtx ctx) = 0;
	virtual void client_did_source(IncomingCtx ctx, int source_file_id) = 0;
	virtual void client_did_list_update(IncomingCtx ctx) = 0;
	virtual void client_did_list_toggle_expand(IncomingCtx ctx, int object_id) = 0;
	virtual void client_did_list_details(IncomingCtx ctx, int object_id) = 0;
};

// クライアントにメッセージを送るもの
class KnowbugSender {
public:
	virtual ~KnowbugSender() = 0;
	//void send_message(const KnowbugMessage& msg);
	virtual void send_initialized_event() = 0;
	virtual void send_terminated_event() = 0;
	virtual void send_continued_event() = 0;
	virtual void send_stopped_event() = 0;
	virtual void send_location_event(size_t source_file_id, size_t line_index) = 0;
	virtual void send_source_event(size_t source_file_id, std::optional<std::u8string_view> source_path_opt, std::optional<std::u8string_view> source_code_opt) = 0;
	virtual void send_list_updated_events(std::vector<HspObjectListDelta> diff) = 0;
	virtual void send_list_details_event(size_t object_id, std::optional<std::u8string> text_opt) = 0;
	virtual void send_output_event(std::u8string output) = 0;
};

extern void knowbug_step_out(HSP3DEBUG* debug);
extern void knowbug_step_over(HSP3DEBUG* debug);
