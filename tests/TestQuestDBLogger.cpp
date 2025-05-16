#include <gtest/gtest.h>
#include "Utils/QuestDBLogger.hpp"

TEST(QuestDBLoggerTest, BasicLogging) {
    QuestDBLogger logger;
    ASSERT_NO_THROW(logger.log("TestMethod", 42));
}