#pragma once
// Thin logging wrapper. Uses OBS blog() when compiled into the plugin,
// falls back to stderr for host unit-test builds.
#include <cstdio>

#if defined(HYPECLIP_HAVE_OBS)
#include <obs-module.h>
#define HC_LOG(level, fmt, ...) blog(level, "[HypeClip] " fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR   0
#define LOG_WARNING 1
#define LOG_INFO    2
#define LOG_DEBUG   3
#define HC_LOG(level, fmt, ...) do { (void)level; fprintf(stderr, "[HypeClip] " fmt "\n", ##__VA_ARGS__); } while(0)
#endif

#define HC_INFO(fmt, ...)  HC_LOG(LOG_INFO,    fmt, ##__VA_ARGS__)
#define HC_WARN(fmt, ...)  HC_LOG(LOG_WARNING, fmt, ##__VA_ARGS__)
#define HC_ERR(fmt, ...)   HC_LOG(LOG_ERROR,   fmt, ##__VA_ARGS__)
