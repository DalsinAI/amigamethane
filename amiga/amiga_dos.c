/* The User drawer for the high scores, made when it isn't there.
 * The AmiCradle Team. GPL 2 or later, like Super Methane Brothers. */
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/dos.h>

int amiga_make_dir(const char *name)
{
	BPTR lock = Lock((CONST_STRPTR)name, ACCESS_READ);
	if (lock)
	{
		UnLock(lock);
		return 1;
	}
	lock = CreateDir((CONST_STRPTR)name);
	if (!lock)
		return 0;
	UnLock(lock);
	return 1;
}
