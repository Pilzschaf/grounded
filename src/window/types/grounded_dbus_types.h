#ifndef GROUNDED_DBUS_TYPES_H
#define GROUNDED_DBUS_TYPES_H

typedef struct DBusConnection DBusConnection;
typedef struct DBusError DBusError;
typedef struct DBusMessage DBusMessage;
typedef struct DBusMessageIter DBusMessageIter;
typedef struct DBusPendingCall DBusPendingCall;

/* boolean size must be fixed at 4 bytes due to wire protocol! */
typedef uint32_t dbus_bool_t;

#define DBUS_TYPE_INVALID       ((int) '\0')
#define DBUS_TYPE_ARRAY         ((int) 'a')
#define DBUS_TYPE_STRING        ((int) 's')
#define DBUS_TYPE_OBJECT_PATH   ((int) 'o')
#define DBUS_TYPE_UINT32        ((int) 'u')
#define DBUS_TYPE_DICT_ENTRY    ((int) 'e')
#define DBUS_TYPE_VARIANT       ((int) 'v')

/**
 * Object representing an exception.
 */
struct DBusError
{
  const char *name;    /**< public error name field */
  const char *message; /**< public error message field */

  unsigned int dummy1 : 1; /**< placeholder */
  unsigned int dummy2 : 1; /**< placeholder */
  unsigned int dummy3 : 1; /**< placeholder */
  unsigned int dummy4 : 1; /**< placeholder */
  unsigned int dummy5 : 1; /**< placeholder */

  void *padding1; /**< placeholder */
};

struct DBusMessageIter
{ 
  void *dummy1;         /**< Don't use this */
  void *dummy2;         /**< Don't use this */
  uint32_t dummy3; /**< Don't use this */
  int dummy4;           /**< Don't use this */
  int dummy5;           /**< Don't use this */
  int dummy6;           /**< Don't use this */
  int dummy7;           /**< Don't use this */
  int dummy8;           /**< Don't use this */
  int dummy9;           /**< Don't use this */
  int dummy10;          /**< Don't use this */
  int dummy11;          /**< Don't use this */
  int pad1;             /**< Don't use this */
  void *pad2;           /**< Don't use this */
  void *pad3;           /**< Don't use this */
};

typedef enum
{
  DBUS_BUS_SESSION,    /**< The login session bus */
  DBUS_BUS_SYSTEM,     /**< The systemwide bus */
  DBUS_BUS_STARTER     /**< The bus that started us, if any */
} DBusBusType;

#endif // GROUNDED_DBUS_TYPES_H