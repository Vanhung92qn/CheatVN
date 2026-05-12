// =====================================================================
//  ScannerBridge.cpp — implementation cầu nối Scanner native↔managed
// =====================================================================

#include "ScannerBridge.h"
#include "MemoryScanner.h"   // native — cheatvn::MemoryScanner, ScanValue, ...

#include <cstring>  // memcpy

using namespace System;
using namespace System::Collections::Generic;

namespace GUI {

    // =================================================================
    //  Helper: encode/decode Int64 raw ⇄ native value
    // =================================================================
    namespace {

        // Encode native ScanValue → Int64 raw bits
        Int64 encodeScanValue(const cheatvn::ScanValue& sv) {
            Int64 bits = 0;
            switch (sv.type) {
                case cheatvn::ValueType::Int8:   bits = (Int64)sv.i8;  break;
                case cheatvn::ValueType::Int16:  bits = (Int64)sv.i16; break;
                case cheatvn::ValueType::Int32:  bits = (Int64)sv.i32; break;
                case cheatvn::ValueType::Int64:  bits = sv.i64;        break;
                case cheatvn::ValueType::Float: {
                    // Copy 4 bytes of float vào 4 bytes thấp của bits
                    int32_t tmp;
                    std::memcpy(&tmp, &sv.f32, sizeof(tmp));
                    bits = (Int64)tmp;
                    break;
                }
                case cheatvn::ValueType::Double:
                    std::memcpy(&bits, &sv.f64, sizeof(bits));
                    break;
                default: break;
            }
            return bits;
        }

        // Decode Int64 raw bits → native ScanValue
        cheatvn::ScanValue decodeScanValue(Int64 bits, ManagedValueType mtype) {
            cheatvn::ScanValue sv;
            sv.type = (cheatvn::ValueType)(int)mtype;
            switch (sv.type) {
                case cheatvn::ValueType::Int8:   sv.i8  = (int8_t)bits;  break;
                case cheatvn::ValueType::Int16:  sv.i16 = (int16_t)bits; break;
                case cheatvn::ValueType::Int32:  sv.i32 = (int32_t)bits; break;
                case cheatvn::ValueType::Int64:  sv.i64 = bits;          break;
                case cheatvn::ValueType::Float: {
                    int32_t tmp = (int32_t)bits;
                    std::memcpy(&sv.f32, &tmp, sizeof(sv.f32));
                    break;
                }
                case cheatvn::ValueType::Double:
                    std::memcpy(&sv.f64, &bits, sizeof(sv.f64));
                    break;
                default: break;
            }
            return sv;
        }

        // Convert 1 native ScanResult → managed ManagedScanResult^
        ManagedScanResult^ wrapResult(const cheatvn::ScanResult& nr) {
            auto m = gcnew ManagedScanResult();
            m->Address = nr.address;
            m->Type = (ManagedValueType)(int)nr.currentValue.type;
            m->CurrentRaw = encodeScanValue(nr.currentValue);
            m->HasPrevious = nr.hasPrevious;
            if (nr.hasPrevious) {
                m->PreviousRaw = encodeScanValue(nr.previousValue);
            } else {
                m->PreviousRaw = 0;
            }
            return m;
        }

        // Format Int64 raw → display string theo type
        String^ formatRaw(Int64 raw, ManagedValueType type) {
            switch (type) {
                case ManagedValueType::Int8:
                    return ((SByte)raw).ToString();
                case ManagedValueType::Int16:
                    return ((Int16)raw).ToString();
                case ManagedValueType::Int32:
                    return ((Int32)raw).ToString();
                case ManagedValueType::Int64:
                    return raw.ToString();
                case ManagedValueType::Float: {
                    int32_t tmp = (int32_t)raw;
                    float f;
                    std::memcpy(&f, &tmp, sizeof(f));
                    return f.ToString("F4");
                }
                case ManagedValueType::Double: {
                    double d;
                    int64_t bits = raw;
                    std::memcpy(&d, &bits, sizeof(d));
                    return d.ToString("F6");
                }
                default: return "?";
            }
        }
    }

    // =================================================================
    //  ManagedScanResult — display property implementations
    // =================================================================
    String^ ManagedScanResult::AddressDisplay::get() {
        // Format thành "0x7FF6A4C81A20" — hexa uppercase với prefix 0x
        return "0x" + Address.ToString("X");
    }

    String^ ManagedScanResult::CurrentDisplay::get() {
        return formatRaw(CurrentRaw, Type);
    }

    String^ ManagedScanResult::PreviousDisplay::get() {
        return HasPrevious ? formatRaw(PreviousRaw, Type) : "—";
    }

    // =================================================================
    //  ScannerBridge — encode helpers
    // =================================================================
    Int64 ScannerBridge::EncodeInt8(SByte v)   { return (Int64)v; }
    Int64 ScannerBridge::EncodeInt16(Int16 v)  { return (Int64)v; }
    Int64 ScannerBridge::EncodeInt32(Int32 v)  { return (Int64)v; }
    Int64 ScannerBridge::EncodeInt64(Int64 v)  { return v; }

    Int64 ScannerBridge::EncodeFloat(float v) {
        int32_t tmp;
        std::memcpy(&tmp, &v, sizeof(tmp));
        return (Int64)tmp;
    }

    Int64 ScannerBridge::EncodeDouble(double v) {
        Int64 bits;
        std::memcpy(&bits, &v, sizeof(bits));
        return bits;
    }

    // =================================================================
    //  TryParseValue — parse text → raw bits
    // =================================================================
    bool ScannerBridge::TryParseValue(
        String^ text, ManagedValueType type, Int64% outRaw)
    {
        outRaw = 0;
        if (String::IsNullOrWhiteSpace(text)) return false;

        try {
            switch (type) {
                case ManagedValueType::Int8: {
                    SByte v = SByte::Parse(text);
                    outRaw = EncodeInt8(v); return true;
                }
                case ManagedValueType::Int16: {
                    Int16 v = Int16::Parse(text);
                    outRaw = EncodeInt16(v); return true;
                }
                case ManagedValueType::Int32: {
                    Int32 v = Int32::Parse(text);
                    outRaw = EncodeInt32(v); return true;
                }
                case ManagedValueType::Int64: {
                    Int64 v = Int64::Parse(text);
                    outRaw = EncodeInt64(v); return true;
                }
                case ManagedValueType::Float: {
                    float v = (float)Double::Parse(text);
                    outRaw = EncodeFloat(v); return true;
                }
                case ManagedValueType::Double: {
                    double v = Double::Parse(text);
                    outRaw = EncodeDouble(v); return true;
                }
                default: return false;
            }
        }
        catch (FormatException^) { return false; }
        catch (OverflowException^) { return false; }
    }

    // =================================================================
    //  FirstScan
    // =================================================================
    List<ManagedScanResult^>^ ScannerBridge::FirstScan(
        IntPtr hProcess,
        ManagedValueType type,
        Int64 targetRaw,
        ManagedScanOperator op)
    {
        // 1) Convert managed args → native
        HANDLE h = (HANDLE)hProcess.ToPointer();
        cheatvn::ScanValue target = decodeScanValue(targetRaw, type);
        cheatvn::ScanOperator nop = (cheatvn::ScanOperator)(int)op;

        // 2) Gọi native scanner (HEAVY — chạy 100% native, không có
        //    managed transition trong inner loop)
        auto nativeResults = cheatvn::MemoryScanner::firstScan(h, target, nop);

        // 3) Convert native vector → managed List
        auto result = gcnew List<ManagedScanResult^>();
        result->Capacity = (int)nativeResults.size();
        for (const auto& nr : nativeResults) {
            result->Add(wrapResult(nr));
        }
        return result;
    }

    // =================================================================
    //  NextScan
    // =================================================================
    List<ManagedScanResult^>^ ScannerBridge::NextScan(
        IntPtr hProcess,
        List<ManagedScanResult^>^ previous,
        ManagedValueType type,
        Int64 targetRaw,
        ManagedScanOperator op)
    {
        HANDLE h = (HANDLE)hProcess.ToPointer();
        cheatvn::ScanValue target = decodeScanValue(targetRaw, type);
        cheatvn::ScanOperator nop = (cheatvn::ScanOperator)(int)op;

        // Convert managed previous → native vector
        std::vector<cheatvn::ScanResult> nativePrev;
        nativePrev.reserve(previous->Count);
        for each (ManagedScanResult^ mr in previous) {
            cheatvn::ScanResult nr;
            nr.address = mr->Address;
            nr.currentValue = decodeScanValue(mr->CurrentRaw, mr->Type);
            nr.previousValue = decodeScanValue(mr->PreviousRaw, mr->Type);
            nr.hasPrevious = mr->HasPrevious;
            nativePrev.push_back(nr);
        }

        auto nativeOut = cheatvn::MemoryScanner::nextScan(h, nativePrev, target, nop);

        auto result = gcnew List<ManagedScanResult^>();
        result->Capacity = (int)nativeOut.size();
        for (const auto& nr : nativeOut) {
            result->Add(wrapResult(nr));
        }
        return result;
    }

    // =================================================================
    //  RefreshValues — cập nhật in-place các CurrentRaw
    // =================================================================
    void ScannerBridge::RefreshValues(
        IntPtr hProcess,
        List<ManagedScanResult^>^ results)
    {
        HANDLE h = (HANDLE)hProcess.ToPointer();

        for each (ManagedScanResult^ mr in results) {
            cheatvn::ValueType nt = (cheatvn::ValueType)(int)mr->Type;
            cheatvn::ScanValue current;
            if (cheatvn::MemoryScanner::readValue(h, mr->Address, nt, current)) {
                mr->CurrentRaw = encodeScanValue(current);
            }
            // Nếu read fail → giữ giá trị cũ (process có thể đã unmap region)
        }
    }

    // =================================================================
    //  WriteValue
    // =================================================================
    bool ScannerBridge::WriteValue(
        IntPtr hProcess,
        UInt64 address,
        ManagedValueType type,
        Int64 valueRaw)
    {
        HANDLE h = (HANDLE)hProcess.ToPointer();
        cheatvn::ScanValue sv = decodeScanValue(valueRaw, type);
        return cheatvn::MemoryScanner::writeValue(h, address, sv);
    }

    // =================================================================
    //  FirstScanBenchmark — chạy scan với cấu hình tùy chỉnh, chỉ trả count
    // =================================================================
    int ScannerBridge::FirstScanBenchmark(
        IntPtr hProcess,
        ManagedValueType type,
        Int64 targetRaw,
        ManagedScanOperator op,
        unsigned int numThreads,
        bool useSimd)
    {
        HANDLE h = (HANDLE)hProcess.ToPointer();
        cheatvn::ScanValue target = decodeScanValue(targetRaw, type);
        cheatvn::ScanOperator nop = (cheatvn::ScanOperator)(int)op;

        cheatvn::MemoryScanner::ScanOptions opts;
        opts.numThreads = numThreads;
        opts.useSimd = useSimd;

        auto results = cheatvn::MemoryScanner::firstScan(h, target, nop, opts);
        return (int)results.size();
    }

    // =================================================================
    //  FindPointersTo — pointer scanner depth 1
    // =================================================================
    List<UInt64>^ ScannerBridge::FindPointersTo(IntPtr hProcess, UInt64 targetAddress)
    {
        HANDLE h = (HANDLE)hProcess.ToPointer();
        auto native = cheatvn::MemoryScanner::findPointersTo(h, targetAddress);

        auto result = gcnew List<UInt64>();
        result->Capacity = (int)native.size();
        for (uint64_t addr : native) {
            result->Add(addr);
        }
        return result;
    }

    // =================================================================
    //  EnumerateRegions — list các vùng nhớ readable cho Heatmap
    // =================================================================
    List<ManagedMemoryRegion^>^ ScannerBridge::EnumerateRegions(IntPtr hProcess)
    {
        HANDLE h = (HANDLE)hProcess.ToPointer();
        auto native = cheatvn::MemoryScanner::enumerateRegions(h);

        auto result = gcnew List<ManagedMemoryRegion^>();
        result->Capacity = (int)native.size();
        for (const auto& r : native) {
            auto m = gcnew ManagedMemoryRegion();
            m->BaseAddress = r.baseAddress;
            m->Size = r.size;
            m->Protection = r.protection;
            m->RegionState = r.state;
            m->RegionType = r.type;
            m->IsReadable = r.isReadable();
            m->IsWritable = r.isWritable();
            m->IsImage = r.isImage();
            m->IsPrivate = r.isPrivate();
            m->IsMapped = (r.type == MEM_MAPPED);
            result->Add(m);
        }
        return result;
    }

    // =================================================================
    //  ReadBytes — đọc N bytes raw cho Hex Viewer
    // =================================================================
    // Pin managed array để truyền pointer cho native ReadProcessMemory.
    // pin_ptr giữ object không bị GC dời chỗ trong scope.
    cli::array<Byte>^ ScannerBridge::ReadBytes(
        IntPtr hProcess, UInt64 address, int count)
    {
        if (count <= 0) return gcnew cli::array<Byte>(0);

        auto buffer = gcnew cli::array<Byte>(count);
        HANDLE h = (HANDLE)hProcess.ToPointer();

        // Pin array → managed pointer cố định trong vùng nhớ trong scope
        pin_ptr<Byte> pin = &buffer[0];

        SIZE_T bytesRead = 0;
        BOOL ok = ReadProcessMemory(
            h,
            reinterpret_cast<LPCVOID>(address),
            pin,
            static_cast<SIZE_T>(count),
            &bytesRead);

        if (!ok && bytesRead == 0) {
            return gcnew cli::array<Byte>(0);
        }

        // Nếu đọc partial, trả về sub-array đúng số bytes thực đọc
        if ((int)bytesRead < count) {
            auto trimmed = gcnew cli::array<Byte>((int)bytesRead);
            System::Array::Copy(buffer, trimmed, (int)bytesRead);
            return trimmed;
        }
        return buffer;
    }

} // namespace GUI
