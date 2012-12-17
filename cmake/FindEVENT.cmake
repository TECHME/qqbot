# -*- cmake -*-

# - Find ev library (libev)
# Find the ev includes and library
# This module defines
#  EVENT_INCLUDE_DIR, where to find db.h, etc.
#  EVENT_LIBRARIES, the libraries needed to use libev.
#  EVENT_FOUND, If false, do not try to use libev.
# also defined, but not for general use are
#  EVENT_LIBRARY, where to find the libev library.

FIND_PATH(EVENT_INCLUDE_DIR event2/event.h
/usr/local/include
/usr/local/include/libevent2
/usr/include
/usr/include/libevent2
)

SET(EVENT_NAMES ${EVENT_NAMES} event)
FIND_LIBRARY(EVENT_LIBRARY
  NAMES ${EVENT_NAMES}
  PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64
  )

IF (EVENT_LIBRARY AND EVENT_INCLUDE_DIR)
    SET(EVENT_LIBRARIES ${EVENT_LIBRARY})
    SET(EVENT_FOUND "YES")
ELSE (EVENT_LIBRARY AND EVENT_INCLUDE_DIR)
  SET(EVENT_FOUND "NO")
ENDIF (EVENT_LIBRARY AND EVENT_INCLUDE_DIR)


IF (EVENT_FOUND)
   IF (NOT EVENT_FIND_QUIETLY)
      MESSAGE(STATUS "Found libevent: ${EVENT_LIBRARIES}")
   ENDIF (NOT EVENT_FIND_QUIETLY)
ELSE (EVENT_FOUND)
   IF (EVENT_FIND_REQUIRED)
      MESSAGE(STATUS "Could not find libevent library")
   ENDIF (EVENT_FIND_REQUIRED)
ENDIF (EVENT_FOUND)

# Deprecated declarations.
SET (NATIVE_EVENT_INCLUDE_PATH ${EVENT_INCLUDE_DIR} )
GET_FILENAME_COMPONENT (NATIVE_EVENT_LIB_PATH ${EVENT_LIBRARY} PATH)

SET (EVENT_LIBRARIES ${EVENT_LIBRARY})

MARK_AS_ADVANCED(
    EVENT_LIBRARIES
    EVENT_INCLUDE_DIR
  )
