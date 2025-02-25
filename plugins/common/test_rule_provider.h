// test_rule_provider.h
#ifndef TEST_RULE_PROVIDER_H
#define TEST_RULE_PROVIDER_H

#include "../../include/action_manager.h"

// 测试回调函数
void test_callback(void* data);

// 初始化测试规则
void init_test_rules(void);

// 注册测试规则提供者
void register_test_rule_provider(void);

#endif // TEST_RULE_PROVIDER_H 