#include <stic.h>

#include <unistd.h> /* rmdir() unlink() */

#include "../../src/compat/fs_limits.h"
#include "../../src/cfg/config.h"
#include "../../src/ui/ui.h"
#include "../../src/utils/fs.h"
#include "../../src/background.h"
#include "../../src/filelist.h"
#include "../../src/fileops.h"
#include "../../src/trash.h"

#include "utils.h"

static void wait_for_bg(void);

static char *saved_cwd;

SETUP()
{
	saved_cwd = save_cwd();
	assert_success(chdir(SANDBOX_PATH));

	view_setup(&lwin);
	assert_non_null(get_cwd(lwin.curr_dir, sizeof(lwin.curr_dir)));

	cfg.use_trash = 0;
}

TEARDOWN()
{
	view_teardown(&lwin);
	restore_cwd(saved_cwd);
}

TEST(marked_files_are_removed_permanently)
{
	for(cfg.use_system_calls = 0; cfg.use_system_calls < 2;
			++cfg.use_system_calls)
	{
		int bg;
		for(bg = 0; bg < 2; ++bg)
		{
			create_empty_file(SANDBOX_PATH "/a");
			create_empty_file(SANDBOX_PATH "/b");

			populate_dir_list(&lwin, 0);
			lwin.dir_entry[0].marked = 1;

			if(!bg)
			{
				(void)delete_files(&lwin, '\0', 0);
			}
			else
			{
				(void)delete_files_bg(&lwin, 0);
				wait_for_bg();
			}

			assert_failure(unlink(SANDBOX_PATH "/a"));
			assert_success(unlink(SANDBOX_PATH "/b"));
		}
	}
}

TEST(files_in_trash_are_not_removed_to_trash)
{
	cfg.use_trash = 1;
	set_trash_dir(lwin.curr_dir);

	create_empty_file(SANDBOX_PATH "/a");

	populate_dir_list(&lwin, 0);
	lwin.dir_entry[0].marked = 1;

	(void)delete_files(&lwin, '\0', 1);
	(void)delete_files_bg(&lwin, 1);
	wait_for_bg();

	assert_success(unlink(SANDBOX_PATH "/a"));
}

TEST(trash_is_not_removed_to_trash)
{
	cfg.use_trash = 1;
	set_trash_dir(SANDBOX_PATH "/trash");

	create_empty_dir(SANDBOX_PATH "/trash");

	populate_dir_list(&lwin, 0);
	lwin.dir_entry[0].marked = 1;

	(void)delete_files(&lwin, '\0', 1);
	(void)delete_files_bg(&lwin, 1);
	wait_for_bg();

	assert_success(rmdir(SANDBOX_PATH "/trash"));
}

TEST(marked_files_are_removed_to_trash)
{
	cfg.use_trash = 1;
	set_trash_dir(SANDBOX_PATH "/trash");

	for(cfg.use_system_calls = 0; cfg.use_system_calls < 2;
			++cfg.use_system_calls)
	{
		int bg;
		for(bg = 0; bg < 2; ++bg)
		{
			create_empty_dir(SANDBOX_PATH "/trash");

			create_empty_file(SANDBOX_PATH "/a");
			create_empty_file(SANDBOX_PATH "/b");

			populate_dir_list(&lwin, 0);
			lwin.dir_entry[2].marked = 1;

			if(!bg)
			{
				(void)delete_files(&lwin, 'x', 1);
			}
			else
			{
				(void)delete_files_bg(&lwin, 1);
				wait_for_bg();
			}

			assert_success(unlink(SANDBOX_PATH "/a"));
			assert_failure(unlink(SANDBOX_PATH "/b"));

			assert_success(unlink(SANDBOX_PATH "/trash/000_b"));
			assert_success(rmdir(SANDBOX_PATH "/trash"));
		}
	}
}

static void
wait_for_bg(void)
{
	int counter = 0;
	while(bg_has_active_jobs())
	{
		usleep(2000);
		if(++counter > 100)
		{
			assert_fail("Waiting for too long.");
			break;
		}
	}
}

/* vim: set tabstop=2 softtabstop=2 shiftwidth=2 noexpandtab cinoptions-=(0 : */
/* vim: set cinoptions+=t0 filetype=c : */
