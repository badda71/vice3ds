#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "3ds_cia.h"

static u64 lastTID = 0LL;
static char *ciaerror = NULL;

void CIA_SetErrorBuffer(char *buf)
{
	ciaerror = buf;
}

Result CIA_LaunchLastTitle()
{
//log_citra("enter %s", __func__);
	if (lastTID == 0LL) return -1;

	Result ret = 0;

    u8 param[0x300];
    u8 hmac[0x20];
	memset(param, 0, sizeof(param));
	memset(hmac, 0, sizeof(hmac));

	aptInit();
	if (R_FAILED(ret = APT_PrepareToDoApplicationJump(0, lastTID, MEDIATYPE_SD))) {
		if (ciaerror) sprintf(ciaerror, "APT_PrepareToDoApplicationJump failed: 0x%lx", ret);
		return ret;
	}
	if (R_FAILED(ret = APT_DoApplicationJump(param, sizeof(param), hmac))) {
		if (ciaerror) sprintf(ciaerror, "APT_DoApplicationJump failed: 0x%lx", ret);
		return ret;
	}
	aptExit();
	return 0;
}

Result CIA_InstallTitle(char *path,
	int (*progress_callback)(int, int)) {
	Result ret = 0;
	u32 bytes_read = 0, bytes_written = 0;
	u64 size = 0, offset = 0;
	Handle dst_handle, src_handle;
	AM_TitleEntry title;
	FS_Archive archive;

//log_citra("enter %s: %s", __func__, path);

	amInit();
	fsInit();

	if (progress_callback) progress_callback(0,0);
	
	if (R_FAILED(ret = FSUSER_OpenArchive(&archive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) {
		if (ciaerror) sprintf(ciaerror, "FSUSER_OpenArchive failed: 0x%lx", ret);
		goto errorexit;
	}

	if (R_FAILED(ret = FSUSER_OpenFile(&src_handle, archive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0))) {
		FSUSER_CloseArchive(archive);
		if (ciaerror) sprintf(ciaerror, "FSUSER_OpenFile failed: 0x%lx", ret);
		goto errorexit;
	}
	
	if (R_FAILED(ret = AM_GetCiaFileInfo(MEDIATYPE_SD, &title, src_handle))) {
		FSFILE_Close(src_handle);
		FSUSER_CloseArchive(archive);
		if (ciaerror) sprintf(ciaerror, "AM_GetCiaFileInfo failed: 0x%lx", ret);
		goto errorexit;
	}
	
	if (R_FAILED(ret = FSFILE_GetSize(src_handle, &size))) {
		FSFILE_Close(src_handle);
		FSUSER_CloseArchive(archive);
		if (ciaerror) sprintf(ciaerror, "FSFILE_GetSize failed: 0x%lx", ret);
		goto errorexit;
	}
	
	if (R_FAILED(ret = AM_StartCiaInstall(MEDIATYPE_SD, &dst_handle))) {
		FSFILE_Close(src_handle);
		FSUSER_CloseArchive(archive);
		if (ciaerror) sprintf(ciaerror, "AM_StartCiaInstall failed: 0x%lx", ret);
		goto errorexit;
	}

	size_t buf_size = 0x10000;
	u8 *buf = malloc(buf_size); // Chunk size

	do {
		memset(buf, 0, buf_size);

		if (R_FAILED(ret = FSFILE_Read(src_handle, &bytes_read, offset, buf, buf_size))) {
			free(buf);
			AM_CancelCIAInstall(dst_handle);
			FSFILE_Close(src_handle);
			FSUSER_CloseArchive(archive);
			if (ciaerror) sprintf(ciaerror, "FSFILE_Read failed: 0x%lx", ret);
			goto errorexit;
		}
		if (R_FAILED(ret = FSFILE_Write(dst_handle, &bytes_written, offset, buf, bytes_read, FS_WRITE_FLUSH))) {
			free(buf);
			AM_CancelCIAInstall(dst_handle);
			FSFILE_Close(src_handle);
			FSUSER_CloseArchive(archive);
			if (ciaerror) sprintf(ciaerror, "FSFILE_Write failed at offset 0x%llx: 0x%lx", offset, ret);
			goto errorexit;
		}

		offset += bytes_read;
		if (progress_callback && progress_callback(size,offset)!=0) {
			free(buf);
			AM_CancelCIAInstall(dst_handle);
			FSFILE_Close(src_handle);
			FSUSER_CloseArchive(archive);
			if (ciaerror) sprintf(ciaerror, "Aborted by user");
			goto errorexit;
		};
	} while(offset < size);

	free(buf);

	if (bytes_read != bytes_written) {
		AM_CancelCIAInstall(dst_handle);
		FSFILE_Close(src_handle);
		FSUSER_CloseArchive(archive);
		if (ciaerror) sprintf(ciaerror, "CIA bytes written mismatch");
		goto errorexit;
	}

	if (R_FAILED(ret = AM_FinishCiaInstall(dst_handle))) {
		AM_CancelCIAInstall(dst_handle);
		FSFILE_Close(src_handle);
		FSUSER_CloseArchive(archive);
		if (ciaerror) sprintf(ciaerror, "AM_FinishCiaInstall failed: 0x%lx", ret);
		goto errorexit;
	}

	FSFILE_Close(src_handle);
	FSUSER_CloseArchive(archive);

	lastTID=title.titleID;

commonexit:
	amExit();
	fsExit();
	return ret;

errorexit:
	ret=-1;
	goto commonexit;
}