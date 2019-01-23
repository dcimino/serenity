#pragma once

#include "FileSystem.h"
#include <AK/ByteBuffer.h>

class DiskBackedFS : public FS {
public:
    virtual ~DiskBackedFS() override;

    DiskDevice& device() { return *m_device; }
    const DiskDevice& device() const { return *m_device; }

    size_t blockSize() const { return m_blockSize; }

protected:
    explicit DiskBackedFS(RetainPtr<DiskDevice>&&);

    void setBlockSize(unsigned);

    ByteBuffer readBlock(unsigned index) const;
    ByteBuffer readBlocks(unsigned index, unsigned count) const;

    bool writeBlock(unsigned index, const ByteBuffer&);
    bool writeBlocks(unsigned index, unsigned count, const ByteBuffer&);

private:
    size_t m_blockSize { 0 };
    RetainPtr<DiskDevice> m_device;
};