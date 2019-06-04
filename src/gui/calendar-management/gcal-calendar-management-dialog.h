/* gcal-source-manager.h
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#pragma once

#include <glib-object.h>
#include "gcal-application.h"
#include "gcal-manager.h"

#include <libecal/libecal.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GCAL_RESPONSE_REMOVE_SOURCE   2

#define GCAL_TYPE_CALENDAR_MANAGEMENT_DIALOG (gcal_calendar_management_dialog_get_type())

G_DECLARE_FINAL_TYPE (GcalCalendarManagementDialog,
                      gcal_calendar_management_dialog,
                      GCAL, CALENDAR_MANAGEMENT_DIALOG,
                      GtkDialog)

/**
 * GcalCalendarManagementDialogMode:
 * @GCAL_CALENDAR_MANAGEMENT_MODE_CREATE: creating a new calendar
 * @GCAL_CALENDAR_MANAGEMENT_MODE_CREATE_WEB: creating a new web-based calendar
 * @GCAL_CALENDAR_MANAGEMENT_MODE_EDIT: editing an existing calendar
 * @GCAL_CALENDAR_MANAGEMENT_MODE_NORMAL: showing the list of calendars and online accounts
 *
 * The current action of the #GcalCalendarManagementDialog
 */
typedef enum
{
  GCAL_CALENDAR_MANAGEMENT_MODE_CREATE,
  GCAL_CALENDAR_MANAGEMENT_MODE_CREATE_WEB,
  GCAL_CALENDAR_MANAGEMENT_MODE_EDIT,
  GCAL_CALENDAR_MANAGEMENT_MODE_NORMAL
} GcalCalendarManagementDialogMode;

void                 gcal_calendar_management_dialog_set_mode    (GcalCalendarManagementDialog     *dialog,
                                                                  GcalCalendarManagementDialogMode  mode);

void                 gcal_calendar_management_dialog_set_source  (GcalCalendarManagementDialog     *dialog,
                                                                  ESource                          *source);

G_END_DECLS