HEADERS += \
    $$PWD/qqmljsast_p.h \
    $$PWD/qqmljsastfwd_p.h \
    $$PWD/qqmljsastvisitor_p.h \
    $$PWD/qqmljsengine_p.h \
    $$PWD/qqmljslexer_p.h \
    $$PWD/qqmljsglobal_p.h \
    $$PWD/qqmljskeywords_p.h

SOURCES += \
    $$PWD/qqmljsast.cpp \
    $$PWD/qqmljsastvisitor.cpp \
    $$PWD/qqmljsengine_p.cpp \
    $$PWD/qqmljslexer.cpp \

CONFIG += qlalr
QLALRSOURCES = $$PWD/qqmljs.g
QMAKE_QLALRFLAGS = --no-debug --qt

OTHER_FILES += $$QLALRSOURCES

# make sure we install the headers generated by qlalr
private_headers.CONFIG += no_check_exist
