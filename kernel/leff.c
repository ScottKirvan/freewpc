
#include <freewpc.h>

#define MACHINE_LAMP_EFFECTS \
	DECL_LEFF (LEFF_AMODE, L_RUNNING, 10, 0, 0, amode_leff) \
	DECL_LEFF (LEFF_FLASH_ALL, L_NORMAL, 100, 0, 0, flash_all_leff) \


/* Declare externs for all of the deff functions */
#define DECL_LEFF(num, flags, pri, b1, b2, fn) \
	extern void fn (void);

MACHINE_LAMP_EFFECTS


/* Now declare the deff table itself */
#undef DECL_LEFF
#define DECL_LEFF(num, flags, pri, b1, b2, fn) \
	[num] = { flags, pri, b1, b2, fn },

static const leff_t leff_table[] = {
	[LEFF_NULL] = { L_NORMAL, 0, 0, 0, NULL },
	MACHINE_LAMP_EFFECTS
};


/* Declare externs for lamp bit matrices used by leffs */
extern __fastram__ U8 lamp_leff1_allocated[NUM_LAMP_COLS];
extern __fastram__ U8 lamp_leff1_matrix[NUM_LAMP_COLS];
extern __fastram__ U8 lamp_leff2_allocated[NUM_LAMP_COLS];
extern __fastram__ U8 lamp_leff2_matrix[NUM_LAMP_COLS];


/** Indicates the id of the leff currently active */
static uint8_t leff_active;

/** Indicates the priority of the leff currently running */
uint8_t leff_prio;

/** Queue of all leffs that have been scheduled to run, even
 * if they are low in priority.  The actively running leff
 * is also in this queue. */
static uint8_t leff_queue[MAX_QUEUED_LEFFS];


uint8_t leff_get_active (void)
{
	return leff_active;
}

static void leff_add_queue (leffnum_t dn)
{
	uint8_t i;

	for (i=0; i < MAX_QUEUED_LEFFS; i++)
		if (leff_queue[i] == dn)
			return;

	for (i=0; i < MAX_QUEUED_LEFFS; i++)
		if (leff_queue[i] == 0)
		{
			leff_queue[i] = dn;
			return;
		}

	fatal (ERR_LEFF_QUEUE_FULL);
}


static void leff_remove_queue (leffnum_t dn)
{
	uint8_t i;
	for (i=0; i < MAX_QUEUED_LEFFS; i++)
		if (leff_queue[i] == dn)
			leff_queue[i] = 0;
}


static leffnum_t leff_get_highest_priority (void)
{
	uint8_t i;
	uint8_t prio = 0;
	uint8_t best = LEFF_NULL;

	for (i=0; i < MAX_QUEUED_LEFFS; i++)
	{
		if (leff_queue[i] != 0)
		{
			const leff_t *leff = &leff_table[leff_queue[i]];
			if (leff->prio > prio)
			{
				prio = leff->prio;
				best = leff_queue[i];
			}
		}
	}

	/* Save the priority of the best leff here */
	leff_prio = prio;

	/* Return the leffnum of the best leff to the caller */
	return best;
}


void leff_create_handler (const leff_t *leff)
{
	lamp_leff1_erase ();
	task_recreate_gid (GID_LEFF, leff->fn);
}


void leff_start (leffnum_t dn)
{
	const leff_t *leff = &leff_table[dn];

	db_puts ("Leff start request for # "); db_puti (dn); db_putc ('\n');

	if (leff->flags & L_RUNNING)
	{
		db_puts ("Adding running leff to queue\n");
		leff_add_queue (dn);
		if (dn == leff_get_highest_priority ())
		{
			/* This is the new active running leff */
			db_puts ("Requested leff is now highest priority\n");
			leff_active = dn;
			leff_create_handler (leff);
		}
		else
		{
			/* This leff cannot run now, because there is a
			 * higher priority leff running. */
			db_puts ("Can't run because higher priority active\n");
		}
	}
	else
	{
		if (leff->prio > leff_prio)
		{
			db_puts ("Restarting quick leff with high pri\n");
			leff_active = dn;
			leff_create_handler (leff);
		}
		else
		{
			db_puts ("Quick leff lacks pri to run\n");
		}
	}
}


void leff_stop (leffnum_t dn)
{
	const leff_t *leff = &leff_table[dn];

	db_puts ("Stopping leff # "); db_puti (dn); db_putc ('\n');

	if (leff->flags & L_RUNNING)
	{
		db_puts ("Remove running leff from queue\n");
		leff_remove_queue (dn);

		leff_start_highest_priority ();
	}
	else
	{
		fatal (ERR_NOT_IMPLEMENTED_YET);
	}
}


void leff_restart (leffnum_t dn)
{
	if (dn == leff_active)
	{
		const leff_t *leff = &leff_table[dn];
		leff_create_handler (leff);
	}
	else
	{
		leff_start (dn);
	}
}


void leff_default (void)
{
	for (;;)
		task_sleep_sec (10);
}


void leff_start_highest_priority (void)
{
	db_puts ("Restarting highest priority leff\n");

	leff_active = leff_get_highest_priority ();
	if (leff_active != LEFF_NULL)
	{
		const leff_t *leff = &leff_table[leff_active];
		db_puts ("Recreating leff task\n");
		leff_create_handler (leff);
	}
	else
	{
		lamp_leff1_erase ();
		lamp_leff2_erase ();
		task_recreate_gid (GID_LEFF, leff_default);
	}
}


void leff_exit (void) __noreturn__
{
	db_puts ("Exiting leff\n");
	task_setgid (GID_LEFF_EXITING);
	leff_remove_queue (leff_active);
	leff_start_highest_priority ();
	task_exit ();
}


void leff_init (void)
{
	leff_prio = 0;
	leff_active = LEFF_NULL;
	memset (leff_queue, 0, MAX_QUEUED_LEFFS);
}


