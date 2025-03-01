#include "devoptab_fs.h"
#include "../wutnewlib/wut_clock.h"

char *
__wut_fs_fixpath(struct _reent *r,
                 const char *path) {
   char *p;
   char *fixedPath;

   if (!path) {
      r->_errno = EINVAL;
      return NULL;
   }

   p = strchr(path, ':') + 1;
   if (!strchr(path, ':')) {
      p = (char *) path;
   }

   if (strlen(p) > PATH_MAX) {
      r->_errno = ENAMETOOLONG;
      return NULL;
   }

   fixedPath = memalign(0x40, PATH_MAX + 1);
   if (!fixedPath) {
      r->_errno = ENOMEM;
      return NULL;
   }

   // cwd is handled by coreinit, so just strip the 'device:' if it exists
   strcpy(fixedPath, p);
   return fixedPath;
}

mode_t __wut_fs_translate_mode(FSStat fileStat) {
   mode_t retMode = 0;

   if ((fileStat.flags & FS_STAT_LINK) == FS_STAT_LINK) {
      retMode |= S_IFLNK;
   } else if ((fileStat.flags & FS_STAT_DIRECTORY) == FS_STAT_DIRECTORY) {
      retMode |= S_IFDIR;
   } else if ((fileStat.flags & FS_STAT_FILE) == FS_STAT_FILE) {
      retMode |= S_IFREG;
   } else if(fileStat.size == 0) {
      // Mounted paths like /vol/external01 have no flags set.
      // If no flag is set and the size is 0, it's a (root) dir
      retMode |= S_IFDIR;
   }  else if (fileStat.size > 0) {
      // Some regular Wii U files have no type info but will have a size
      retMode |= S_IFREG;
   }

   mode_t ownerFlags = (fileStat.mode & (FS_MODE_READ_OWNER | FS_MODE_WRITE_OWNER | FS_MODE_EXEC_OWNER)) >> 2;
   mode_t groupFlags = (fileStat.mode & (FS_MODE_READ_GROUP | FS_MODE_WRITE_GROUP | FS_MODE_EXEC_GROUP)) >> 1;
   mode_t userFlags = (fileStat.mode & (FS_MODE_READ_OTHER | FS_MODE_WRITE_OTHER | FS_MODE_EXEC_OTHER));

   return retMode | ownerFlags | groupFlags | userFlags;
}

time_t __wut_fs_translate_time(FSTime timeValue) {
   return (timeValue /1000000) + EPOCH_DIFF_SECS(WIIU_FSTIME_EPOCH_YEAR);
}

int
__wut_fs_translate_error(FSStatus error) {
   switch ((int) error) {
      case FS_STATUS_END:
         return ENOENT;
      case FS_STATUS_CANCELLED:
         return ECANCELED;
      case FS_STATUS_EXISTS:
         return EEXIST;
      case FS_STATUS_MEDIA_ERROR:
         return EIO;
      case FS_STATUS_NOT_FOUND:
         return ENOENT;
      case FS_STATUS_PERMISSION_ERROR:
         return EPERM;
      case FS_STATUS_STORAGE_FULL:
         return ENOSPC;
      case FS_STATUS_FILE_TOO_BIG:
         return EFBIG;
      case FS_STATUS_NOT_DIR:
         return ENOTDIR;
      case FS_STATUS_NOT_FILE:
         return EISDIR;
      case FS_STATUS_MAX:
         return ENFILE;
      case FS_STATUS_ACCESS_ERROR:
         return EACCES;
      case FS_STATUS_JOURNAL_FULL:
         return ENOSPC;
      case FS_STATUS_UNSUPPORTED_CMD:
         return ENOTSUP;
      case FS_STATUS_MEDIA_NOT_READY:
         return EOWNERDEAD;
      case FS_STATUS_ALREADY_OPEN:
      case FS_STATUS_CORRUPTED:
      case FS_STATUS_FATAL_ERROR:
         return EIO;
   }
   return (int) error;
}
