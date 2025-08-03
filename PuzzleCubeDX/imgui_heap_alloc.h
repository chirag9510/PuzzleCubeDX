// srv heap allocator for imgui
#pragma once
#include "stdafx.h"

#include "./imgui/imgui.h"
#include "./imgui/imgui_impl_win32.h"
#include "./imgui/imgui_impl_dx12.h"

// Simple free list based allocator
struct ImguiheapAlloc
{
    ID3D12DescriptorHeap* dscHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  heapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE heapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE heapStartGpu;
    UINT                        heapHandleIncrement;
    ImVector<int>               freeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
    {
        if (freeIndices.empty())
            OutputDebugString(L"BitchesLeave \n\n");

        IM_ASSERT(1);
        this->dscHeap = heap;
        D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
        heapType = desc.Type;
        heapStartCpu = heap->GetCPUDescriptorHandleForHeapStart();
        heapStartGpu = heap->GetGPUDescriptorHandleForHeapStart();
        heapHandleIncrement = device->GetDescriptorHandleIncrementSize(heapType);
        freeIndices.reserve((int)desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
            freeIndices.push_back(n - 1);
    }
    void Destroy()
    {
        dscHeap = nullptr;
        freeIndices.clear();
    }
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
    {
        IM_ASSERT(freeIndices.Size > 0);
        int idx = freeIndices.back();
        freeIndices.pop_back();
        out_cpu_desc_handle->ptr = heapStartCpu.ptr + (idx * heapHandleIncrement);
        out_gpu_desc_handle->ptr = heapStartGpu.ptr + (idx * heapHandleIncrement);
    }
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
    {
        int cpu_idx = (int)((out_cpu_desc_handle.ptr - heapStartCpu.ptr) / heapHandleIncrement);
        int gpu_idx = (int)((out_gpu_desc_handle.ptr - heapStartGpu.ptr) / heapHandleIncrement);
        IM_ASSERT(cpu_idx == gpu_idx);
        freeIndices.push_back(cpu_idx);
    }
};

