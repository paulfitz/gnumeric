/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * analysis-normality.c:
 *
 *
 * Author:
 *   Andreas J. Guelzow  <aguelzow@pyrshep.ca>
 *
 * (C) Copyright 2009 by Andreas J. Guelzow  <aguelzow@pyrshep.ca>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gnumeric-config.h>
#include <glib/gi18n-lib.h>
#include "gnumeric.h"
#include "analysis-normality.h"
#include "analysis-tools.h"
#include "value.h"
#include "ranges.h"
#include "expr.h"
#include "func.h"
#include "numbers.h"
#include "sheet-object-graph.h"
#include "graph.h"
#include <goffice/goffice.h>
#include "sheet.h"


static gboolean
analysis_tool_normality_engine_run (data_analysis_output_t *dao,
				      analysis_tools_data_normality_t *info)
{
	guint   col;
	GSList *data = info->base.input;
	GnmFunc *fd;
	GnmFunc *fd_if;

	char const *fdname;
	char const *testname;
	char const *n_comment;

	switch (info->type) {
	case 'a':
	default:
		fdname = "ADTEST";
		testname = N_("Anderson-Darling Test");
		n_comment = N_("For the Anderson-Darling Test\n"
			       "the size the sample must be at\n"
			       "least 8.");
	}

	fd = gnm_func_lookup_or_add_placeholder 
		(fdname, dao->sheet ? dao->sheet->workbook : NULL, FALSE);
	gnm_func_ref (fd);
	fd_if = gnm_func_lookup_or_add_placeholder 
		("IF", dao->sheet ? dao->sheet->workbook : NULL, FALSE);
	gnm_func_ref (fd_if);

	dao_set_italic (dao, 0, 0, 0, 5);
        dao_set_cell (dao, 0, 0, _(testname));

	/* xgettext:
	 * Note to translators: in the following string and others like it,
	 * the "/" is a separator character that can be changed to anything
	 * if the translation needs the slash; just use, say, "|" instead.
	 *
	 * The items are bundled like this to increase translation context.
	 */
        set_cell_text_col (dao, 0, 1, _("/Alpha"
					"/p-Value"
					"/Statistic"
					"/N"
					"/Conclusion"));

	dao_set_cell_comment (dao, 0, 4, _(n_comment));

	for (col = 1; data != NULL; data = data->next, col++) {
		GnmValue *val_org = value_dup (data->data);

		/* Note that analysis_tools_write_label may modify val_org */
		dao_set_italic (dao, col, 0, col, 0);
		analysis_tools_write_label (val_org, dao, &info->base, 
					    col, 0, col);
		if (col == 1)
			dao_set_cell_float (dao, col, 1, info->alpha);
		else
			dao_set_cell_expr (dao, col, 1, 
					   make_cellref (1 - col, 0));

		dao_set_array_expr (dao, col, 2, 1, 3,  
				    gnm_expr_new_funcall1 (fd, gnm_expr_new_constant (val_org)));
		dao_set_cell_expr (dao, col, 5,
				   gnm_expr_new_funcall3 
				   (fd_if, gnm_expr_new_binary 
				    (make_cellref (0, -4), 
				     GNM_EXPR_OP_GTE, 
				     make_cellref (0, -3)),
				    gnm_expr_new_constant (value_new_string (_("Not normal"))), 
				    gnm_expr_new_constant (value_new_string (_("Possibly normal")))));
	}

	gnm_func_unref (fd);
	gnm_func_unref (fd_if);

	dao_autofit_columns (dao);
	dao_redraw_respan (dao);
	return 0;
}

gboolean
analysis_tool_normality_engine (data_analysis_output_t *dao, gpointer specs,
				   analysis_tool_engine_t selector, gpointer result)
{
	analysis_tools_data_normality_t *info = specs;

	switch (selector) {
	case TOOL_ENGINE_UPDATE_DESCRIPTOR:
		return (dao_command_descriptor (dao, _("Normality Test (%s)"), result)
			== NULL);
	case TOOL_ENGINE_UPDATE_DAO:
		prepare_input_range (&info->base.input, info->base.group_by);
		dao_adjust (dao, 1 + g_slist_length (info->base.input), 6);
		return FALSE;
	case TOOL_ENGINE_CLEAN_UP:
		return analysis_tool_generic_clean (specs);
	case TOOL_ENGINE_LAST_VALIDITY_CHECK:
		return FALSE;
	case TOOL_ENGINE_PREPARE_OUTPUT_RANGE:
		dao_prepare_output (NULL, dao, _("Normality Test"));
		return FALSE;
	case TOOL_ENGINE_FORMAT_OUTPUT_RANGE:
		return dao_format_output (dao, _("Normality Test"));
	case TOOL_ENGINE_PERFORM_CALC:
	default:
		return analysis_tool_normality_engine_run (dao, specs);
	}
	return TRUE;  /* We shouldn't get here */
}
