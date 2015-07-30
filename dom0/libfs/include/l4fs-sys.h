#pragma once

namespace L4fs
{
  namespace Svr_
    {
      enum
	{
	  Init, Open, Close, Pread, Pwrite, Pgetdents, Fstat64, Ftruncate64,
          Rename, Unlink, Rmdir, Mkdir, Fsync, Get_dataspace, Umount, Sync, Symlink, Link, Readlink
	};
    };
};
