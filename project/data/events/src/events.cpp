#include "kumo/events/events.h"

namespace kumo {

const char* event_type_name(const Event& event) {
    return std::visit([](const auto& e) -> const char* {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, UpdateRoomEvent>)          return "update_room";
        else if constexpr (std::is_same_v<T, UpdateOnlineEvent>)   return "update_online";
        else if constexpr (std::is_same_v<T, DanmuMessage>)        return "new_danmu";
        else if constexpr (std::is_same_v<T, GiftMessage>)         return "new_gift";
        else if constexpr (std::is_same_v<T, GuardMessage>)        return "new_guard";
        else if constexpr (std::is_same_v<T, SuperChatMessage>)    return "new_superchat";
        else if constexpr (std::is_same_v<T, InteractMessage>)     return "new_interact";
        else if constexpr (std::is_same_v<T, EntryEffectMessage>)  return "new_entry_effect";
        else if constexpr (std::is_same_v<T, RoomChangeMessage>)   return "room_change";
        else if constexpr (std::is_same_v<T, ConfigChangedEvent>)  return "config_changed";
        else if constexpr (std::is_same_v<T, ConfigLoadedEvent>)   return "config_loaded";
        else if constexpr (std::is_same_v<T, DetailUpdateEvent>)   return "detail_update";
        else if constexpr (std::is_same_v<T, LiveStartEvent>)      return "live_start";
        else if constexpr (std::is_same_v<T, LiveEndEvent>)        return "live_end";
        else if constexpr (std::is_same_v<T, ConnectionStatusEvent>) return "connection_status";
        else if constexpr (std::is_same_v<T, LoginStatusChangedEvent>) return "login_status_changed";
        else if constexpr (std::is_same_v<T, RequestQrLoginEvent>) return "request_qr_login";
        else if constexpr (std::is_same_v<T, QrCodeGeneratedEvent>) return "qr_code_generated";
        else if constexpr (std::is_same_v<T, QrLoginStatusEvent>)  return "qr_login_status";
        else if constexpr (std::is_same_v<T, RequestLogoutEvent>)  return "request_logout";
        else if constexpr (std::is_same_v<T, RtmpInfoEvent>)       return "rtmp_info";
        else if constexpr (std::is_same_v<T, FaceAuthRequiredEvent>) return "face_auth_required";
        else if constexpr (std::is_same_v<T, ClearDanmuListEvent>) return "clear_danmu_list";
        else if constexpr (std::is_same_v<T, UserInfoFetchedEvent>) return "user_info_fetched";
        else if constexpr (std::is_same_v<T, AudienceListFetchedEvent>) return "audience_list_fetched";
        else if constexpr (std::is_same_v<T, GuardListFetchedEvent>) return "guard_list_fetched";
        else if constexpr (std::is_same_v<T, DataClearedEvent>)    return "data_cleared";
        else if constexpr (std::is_same_v<T, UpdateCheckResultEvent>) return "update_check_result";
        else if constexpr (std::is_same_v<T, WarningMessage>)      return "warning";
        else if constexpr (std::is_same_v<T, CutOffMessage>)       return "cut_off";
        else return "unknown";
    }, event);
}

// ============================================================================
// EventBus
// ============================================================================

EventBus::EventBus() : m_inner(std::make_shared<Inner>()) {}

void EventBus::emit(const Event& event) {
    m_inner->sender.send(event);

    const char* type_name = event_type_name(event);
    std::shared_lock lock(m_inner->handlers_mutex);
    auto it = m_inner->handlers.find(type_name);
    if (it != m_inner->handlers.end()) {
        for (auto& h : it->second) h(event);
    }
    auto all_it = m_inner->handlers.find("*");
    if (all_it != m_inner->handlers.end()) {
        for (auto& h : all_it->second) h(event);
    }
}

void EventBus::on(const std::string& event_type, Handler handler) {
    std::unique_lock lock(m_inner->handlers_mutex);
    m_inner->handlers[event_type].push_back(std::move(handler));
}

void EventBus::on_all(Handler handler) {
    on("*", std::move(handler));
}

std::shared_ptr<BroadcastChannel<Event>::Receiver> EventBus::subscribe() {
    return m_inner->sender.subscribe();
}

// ============================================================================
// EventType
// ============================================================================

const char* event_type_str(EventType type) {
    switch (type) {
        case EventType::UpdateRoom:       return "update_room";
        case EventType::UpdateOnline:     return "update_online";
        case EventType::NewDanmu:         return "new_danmu";
        case EventType::NewGift:          return "new_gift";
        case EventType::NewGuard:         return "new_guard";
        case EventType::NewSuperChat:     return "new_superchat";
        case EventType::NewInteract:      return "new_interact";
        case EventType::NewEntryEffect:   return "new_entry_effect";
        case EventType::RoomChange:       return "room_change";
        case EventType::ConfigChanged:    return "config_changed";
        case EventType::DetailUpdate:     return "detail_update";
        case EventType::LiveStart:        return "live_start";
        case EventType::LiveEnd:          return "live_end";
        case EventType::ConnectionStatus: return "connection_status";
        case EventType::LoginStatusChanged: return "login_status_changed";
        case EventType::RequestQrLogin:   return "request_qr_login";
        case EventType::QrCodeGenerated:  return "qr_code_generated";
        case EventType::QrLoginStatus:    return "qr_login_status";
        case EventType::RequestLogout:    return "request_logout";
        case EventType::ClearDanmuList:   return "clear_danmu_list";
        case EventType::UserInfoFetched:  return "user_info_fetched";
        case EventType::AudienceListFetched: return "audience_list_fetched";
        case EventType::GuardListFetched: return "guard_list_fetched";
        default: return "unknown";
    }
}

} // namespace kumo
