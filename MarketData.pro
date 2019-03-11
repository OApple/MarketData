TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    property.cpp \
    dataprocessor.cpp \
    mdspi.cpp \
    util.cpp \
    traderspi.cpp

OTHER_FILES += \
    CSHFETraderSpi.o \
    DataProcessor.o \
    FollowOrderPro \
    MdSpi.o \
    property.o \
    TimeProcesser.o \
    TraderSpi.o \
    start.sh \
    Makefile \
    MarketData.pro.user

HEADERS += \
    property.h \
    dataprocessor.h \
    mysqlconnectpool.h \
    mdspi.h \
    util.h \
    traderspi.h \
    instrument.h


LIBS += -L$$PWD/../lib/   -lthostmduserapi  -lthosttraderapi

INCLUDEPATH += $$PWD/../include


#LIBS += -L$$PWD/../glog_0_3_3/ -lglog
#INCLUDEPATH += $$PWD/../glog_0_3_3
LIBS += -lglog
INCLUDEPATH += $$PWD/../SQLiteCpp/include


LIBS += -L$$PWD/../boost_1_61_0/stage/lib/ -lboost_system -lboost_date_time -lboost_system -lboost_thread -lpthread -lboost_chrono  -lboost_locale -lboost_filesystem

INCLUDEPATH += $$PWD/../boost_1_61_0/


LIBS +=  -licui18n -licuuc -licudata  -ldl -lhiredis  -lSQLiteCpp -L/usr/local/lib -lsqlite3

LIBS += -L/usr/local/lib -lmysqlpp

INCLUDEPATH += /usr/local/include/mysql++ /usr/include/mysql

DEPENDPATH += /usr/local/include/mysql++

