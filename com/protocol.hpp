#pragma once

enum class ReturnCode : int {
    Succeeded,
    BadRequest,
    FailedProcess,
};

struct BaseHeader {
    int version;
    int bodyLength;
    ReturnCode rc;
};

struct AsciiartHeader : BaseHeader {
    int requestedWidth;
};