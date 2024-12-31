
X(dbus_bus_get, DBusConnection*, (DBusBusType type, DBusError *error))
X(dbus_connection_unref, void, (DBusConnection *connection))
X(dbus_connection_flush, void, (DBusConnection *connection))
X(dbus_bus_add_match, void, (DBusConnection *connection, const char *rule, DBusError *error))
X(dbus_error_init, void, (DBusError *error))
X(dbus_error_is_set, dbus_bool_t, (const DBusError *error))
X(dbus_error_free, void, (DBusError *error))
X(dbus_message_new_method_call, DBusMessage*, (const char  *bus_name, const char  *path, const char  *iface, const char  *method))
X(dbus_message_iter_init_append, void, (DBusMessage *message, DBusMessageIter *iter))
X(dbus_message_iter_append_basic, dbus_bool_t, (DBusMessageIter *iter, int type, const void *value))
X(dbus_message_iter_open_container, dbus_bool_t, (DBusMessageIter *iter, int type, const char *contained_signature,DBusMessageIter *sub))
X(dbus_message_iter_close_container, dbus_bool_t, (DBusMessageIter *iter, DBusMessageIter *sub))
X(dbus_connection_send_with_reply_and_block, DBusMessage*, (DBusConnection *connection, DBusMessage* message, int timeout_milliseconds, DBusError *error))
X(dbus_message_iter_init, dbus_bool_t, (DBusMessage *message, DBusMessageIter *iter))
X(dbus_message_iter_get_arg_type, int, (DBusMessageIter *iter))
X(dbus_message_iter_get_basic, void, (DBusMessageIter *iter, void *value))
X(dbus_message_unref, void, (DBusMessage *message))
X(dbus_pending_call_block, void, (DBusPendingCall *pending))
X(dbus_connection_send_with_reply, dbus_bool_t, (DBusConnection *connection, DBusMessage *message, DBusPendingCall **pending_return, int timeout_milliseconds))
X(dbus_pending_call_steal_reply, DBusMessage *, (DBusPendingCall *pending))
X(dbus_pending_call_unref, void, (DBusPendingCall *pending))
X(dbus_connection_read_write, dbus_bool_t, (DBusConnection *connection, int timeout_milliseconds))
X(dbus_connection_pop_message, DBusMessage *, (DBusConnection *connection))
X(dbus_message_is_signal, dbus_bool_t, (DBusMessage *message, const char *iface, const char *signal_name))
X(dbus_message_get_path, const char *, (DBusMessage *message))
X(dbus_message_iter_next, dbus_bool_t, (DBusMessageIter *iter))
X(dbus_message_iter_recurse, void, (DBusMessageIter *iter, DBusMessageIter *sub))
X(dbus_bus_remove_match, void, (DBusConnection *connection, const char *rule, DBusError *error))
X(dbus_bus_get_unique_name, const char *, (DBusConnection *connection))
X(dbus_message_iter_get_element_count, int, (DBusMessageIter *iter))