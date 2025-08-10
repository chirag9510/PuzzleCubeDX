#pragma once
#include "stdafx.h"

#include <stdexcept>

using Microsoft::WRL::ComPtr;

inline float roundoff(float val, float floor) {
    if ((val > 0.f && val < 0.1f) || (val < 0.f && val > -0.1f)) {
        return 0.f;
    }
    else if (val > 0.f)
        return floor;
    else if (val < 0.f)
        return -floor;
    else
        return 0.f;
}

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

inline UINT calcConstantBufferByteSize(UINT size) {
    // get cbuffer size in rounded off to a multiple of 256
    return (size + 255) & ~255;
}