/*
 * (C) Copyright 2010 Barnes & Noble, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/*
 * This file provides a simple 'calc' function that performs
 * simple math operations.
 */

#include <common.h>
#include <config.h>
#include <command.h>

#ifndef DISABLE_CALC

enum operation {
	PLUS,
	MINUS,
	TIMES,
	DIVIDE,
	SHL,
	SHR,
	MODULUS,
	AND,
	OR,
	NOT,
	XOR
};

struct op_tbl_s {
	char	*op;		/* operator string */
	enum operation	opcode;	/* internal representation of opcode */
};

typedef struct op_tbl_s op_tbl_t;

static const op_tbl_t op_table [] = {
	{ "+", PLUS },
	{ "-"  , MINUS },
	{ "*", TIMES },
	{ "/"  , DIVIDE },
	{ "<<", SHL },
	{ ">>" , SHR },
	{ "%", MODULUS },
	{ "&" , AND },
	{ "|" , OR },
	{ "^" , XOR },
/*	{ "~", NOT }, */ /* Only binary operations currently supported */
};

#define op_tbl_size (sizeof(op_table)/sizeof(op_table[0]))

extern int cmd_get_data_size(char* arg, int default_size);

static long evalexp(char *s, int w)
{
	long l, *p;

	/* if the parameter starts with a * then assume is a pointer to the value we want */
	if (s[0] == '*') {
		p = (long *)simple_strtoul(&s[1], NULL, 16);
		l = *p;
	} else {
		l = simple_strtoul(s, NULL, 16);
	}

	return (l & ((1 << (w * 8)) - 1));
}

static int arithcalc (char *s, char *t, int op, int w)
{
	long l, r;

	l = evalexp (s, w);
	r = evalexp (t, w);

	switch (op) {
	case PLUS: return (l + r);
	case MINUS: return (l - r);
	case TIMES: return (l * r);
	case DIVIDE: return (l / r);
	case SHL: return (l << r);
	case SHR: return (l >> r);
	case MODULUS: return (l % r);
	case AND: return (l & r);
	case OR: return (l | r);
	case XOR: return (l ^ r);
	}
	return (0);
}

static int binary_test (char *op, char *arg1, char *arg2, int w)
{
	int len, i;
	op_tbl_t *optp;

	len = strlen(op);

	for (optp = (op_tbl_t *)&op_table, i = 0;
	     i < op_tbl_size;
	     optp++, i++) {

		if ((strncmp (op, optp->op, len) == 0) && (len == strlen (optp->op))) {
			if (w == 0)
				w = 4;	/* default to integer */
			return (arithcalc (arg1, arg2, optp->opcode, w));
		}
	}

	printf("Unknown operator '%s'\n", op);
	return 0;	/* op code not found */
}

/* command line interface to the calc command */
int do_calc ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[] )
{
	int	value, w;
	int	*addr;

	/* Validate arguments */
	if (argc < 4){
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	/* Check for a data width specification.
	 * Defaults to long (4) if no specification.
	 * Uses -2 as 'width' for .s (string) so as not to upset existing code
	 */
	switch (w = cmd_get_data_size(argv[0], 4)) {
	case 1:
	case 2:
	case 4:
		value = binary_test (argv[2], argv[1], argv[3], w);
		break;
	case -2:
		value = binary_test (argv[2], argv[1], argv[3], 0);
		break;
	case -1:
	default:
		puts("Invalid data width specifier\n");
		value = 0;
		break;
	}
	printf("%d\n", value);
	if (argc > 4) {
		if (w != 4) {
			puts("Unsupported data width specifier for output\n");
		} else {
			addr = (int *)simple_strtoul(argv[4], NULL, 16);
			*addr = value;
		}
	}

	return value;
}

U_BOOT_CMD(
	calc, 5, 0, do_calc,
	"calc\t- perform mathematical operation\n",
	"[.b, .w, .l] [*]value1 <op> [*]value2 [[*]output]\n"
);
#endif
