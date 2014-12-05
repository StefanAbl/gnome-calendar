/* -*- mode: c; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * gcal-application.c
 * Copyright (C) 2012 Erick Pérez Castellanos <erickpc@gnome.org>
 *
 * gnome-calendar is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gnome-calendar is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gcal-application.h"
#include "gcal-window.h"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#define CSS_FILE "resource:///org/gnome/calendar/gtk-styles.css"

typedef struct
{
  GtkWidget      *window;

  GSettings      *settings;
  GcalManager    *manager;

  GtkCssProvider *provider;
} GcalApplicationPrivate;

static void     gcal_application_finalize             (GObject                 *object);

static void     gcal_application_activate             (GApplication            *app);

static void     gcal_application_startup              (GApplication            *app);

static gint     gcal_application_command_line         (GApplication            *app,
                                                       GApplicationCommandLine *command_line);

static void     gcal_application_set_app_menu         (GApplication            *app);

static void     gcal_application_create_new_event     (GSimpleAction           *new_event,
                                                       GVariant                *parameter,
                                                       gpointer                 app);

static void     gcal_application_launch_search        (GSimpleAction           *search,
                                                       GVariant                *parameter,
                                                       gpointer                 app);

static void     gcal_application_show_about           (GSimpleAction           *simple,
                                                       GVariant                *parameter,
                                                       gpointer                 user_data);

static void     gcal_application_sync                 (GSimpleAction           *sync,
                                                       GVariant                *parameter,
                                                       gpointer                 app);

static void     gcal_application_quit                 (GSimpleAction           *simple,
                                                       GVariant                *parameter,
                                                       gpointer                 user_data);

G_DEFINE_TYPE_WITH_PRIVATE (GcalApplication, gcal_application, GTK_TYPE_APPLICATION);

static gboolean show_version = FALSE;

static GOptionEntry gcal_application_goptions[] = {
  {
    "version", 'v', 0,
    G_OPTION_ARG_NONE, &show_version,
    N_("Display version number"), NULL
  },
  { NULL }
};

static const GActionEntry gcal_app_entries[] = {
  { "new",    gcal_application_create_new_event },
  { "sync",   gcal_application_sync },
  { "search", gcal_application_launch_search },
  { "about",  gcal_application_show_about },
  { "quit",   gcal_application_quit },
};

static void
gcal_application_class_init (GcalApplicationClass *klass)
{
  GObjectClass *object_class;
  GApplicationClass *application_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = gcal_application_finalize;

  application_class = G_APPLICATION_CLASS (klass);
  application_class->activate = gcal_application_activate;
  application_class->startup = gcal_application_startup;
  application_class->command_line = gcal_application_command_line;
}

static void
gcal_application_init (GcalApplication *self)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (self);
  priv->settings = g_settings_new ("org.gnome.calendar");
}

static void
gcal_application_finalize (GObject *object)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (GCAL_APPLICATION (object));

  g_clear_object (&(priv->settings));
  g_clear_object (&(priv->provider));
  g_clear_object (&(priv->manager));

  G_OBJECT_CLASS (gcal_application_parent_class)->finalize (object);
}

static void
gcal_application_activate (GApplication *application)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (GCAL_APPLICATION (application));

  if (priv->window != NULL)
    {
      gtk_window_present (GTK_WINDOW (priv->window));
    }
  else
    {
      priv->window =
        gcal_window_new_with_view (GCAL_APPLICATION (application),
                                   g_settings_get_enum (priv->settings,
                                                        "active-view"));
      g_settings_bind (priv->settings,
                       "active-view",
                       priv->window,
                       "active-view",
                       G_SETTINGS_BIND_SET | G_SETTINGS_BIND_GET);

      gtk_widget_show_all (priv->window);
    }
}

static void
gcal_application_startup (GApplication *app)
{
  GcalApplicationPrivate *priv;
  GFile* css_file;
  GError *error;

  priv = gcal_application_get_instance_private (GCAL_APPLICATION (app));

  G_APPLICATION_CLASS (gcal_application_parent_class)->startup (app);

  if (priv->provider == NULL)
   {
     priv->provider = gtk_css_provider_new ();
     gtk_style_context_add_provider_for_screen (
         gdk_screen_get_default (),
         GTK_STYLE_PROVIDER (priv->provider),
         G_MAXUINT);

     error = NULL;
     css_file = g_file_new_for_uri (CSS_FILE);
     gtk_css_provider_load_from_file (priv->provider, css_file, &error);
     if (error != NULL)
       {
         g_warning ("Error loading stylesheet from file %s. %s",
                    CSS_FILE,
                    error->message);
       }

     g_object_set (gtk_settings_get_default (),
                   "gtk-application-prefer-dark-theme", FALSE,
                   NULL);

     g_object_unref (css_file);
     g_object_unref (priv->provider);
   }

  priv->manager = gcal_manager_new ();

  gcal_application_set_app_menu (app);
}

static gint
gcal_application_command_line (GApplication            *app,
                               GApplicationCommandLine *command_line)
{
  GOptionContext *context;
  gchar **argv;
  GError *error;
  gint argc;

  argv = g_application_command_line_get_arguments (command_line, &argc);
  context = g_option_context_new (N_("- Calendar management"));
  g_option_context_add_main_entries (context, gcal_application_goptions, GETTEXT_PACKAGE);

  error = NULL;
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_critical ("Failed to parse argument: %s", error->message);
      g_error_free (error);
      g_option_context_free (context);
      return 1;
    }

  if (show_version)
    {
      g_print ("gnome-calendar: Version %s\n", PACKAGE_VERSION);
      return 0;
    }

  g_option_context_free (context);
  g_strfreev (argv);

  g_application_activate (app);

  return 0;
}

static void
gcal_application_set_app_menu (GApplication *app)
{
  GtkBuilder *builder;
  GMenuModel *appmenu;

  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder, "/org/gnome/calendar/menus.ui", NULL);

  appmenu = (GMenuModel *)gtk_builder_get_object (builder, "appmenu");

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   gcal_app_entries,
                                   G_N_ELEMENTS (gcal_app_entries),
                                   app);

  gtk_application_set_app_menu (GTK_APPLICATION (app), appmenu);

  g_object_unref (builder);
}

static void
gcal_application_create_new_event (GSimpleAction *new_event,
                                   GVariant      *parameter,
                                   gpointer       app)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (GCAL_APPLICATION (app));

  gcal_window_new_event (GCAL_WINDOW (priv->window));
}

static void
gcal_application_sync (GSimpleAction *sync,
                       GVariant      *parameter,
                       gpointer       app)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (GCAL_APPLICATION (app));

  gcal_manager_refresh (priv->manager);
}

static void
gcal_application_launch_search (GSimpleAction *search,
                                GVariant      *parameter,
                                gpointer       app)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (GCAL_APPLICATION (app));

  gcal_window_set_search_mode (GCAL_WINDOW (priv->window), TRUE);
}

static void
gcal_application_show_about (GSimpleAction *simple,
                             GVariant      *parameter,
                             gpointer       user_data)
{
  GcalApplicationPrivate *priv;
  char *copyright;
  GDateTime *date;
  int created_year = 2012;
  const gchar *authors[] = {
    "Erick Pérez Castellanos <erickpc@gnome.org>",
    "Georges Basile Stavracas Neto <georges.stavracas@gmail.com>",
    NULL
  };
  const gchar *artists[] = {
    "Jakub Steiner <jimmac@gmail.com>",
    "Lapo Calamandrei <calamandrei@gmail.com>",
    "Reda Lazri <the.red.shortcut@gmail.com>",
    "William Jon McCann <jmccann@redhat.com>",
    NULL
  };

  priv = gcal_application_get_instance_private (GCAL_APPLICATION (user_data));
  date = g_date_time_new_now_local ();

  if (g_date_time_get_year (date) == created_year)
    {
      copyright = g_strdup_printf (_("Copyright \xC2\xA9 %Id "
                                     "The Calendar authors"),
                                   created_year);
    }
  else
    {
      copyright = g_strdup_printf (_("Copyright \xC2\xA9 %Id\xE2\x80\x93%Id "
                                     "The Calendar authors"),
                                   created_year, g_date_time_get_year (date));
    }

  gtk_show_about_dialog (GTK_WINDOW (priv->window),
                         "program-name", _("Calendar"),
                         "version", VERSION,
                         "copyright", copyright,
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "authors", authors,
                         "artists", artists,
                         "logo-icon-name", "gnome-calendar",
                         "translator-credits", _("translator-credits"),
                         NULL);
  g_free (copyright);
  g_date_time_unref (date);
}

static void
gcal_application_quit (GSimpleAction *simple,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  GcalApplicationPrivate *priv;

  priv =
    gcal_application_get_instance_private (GCAL_APPLICATION (user_data));

  gtk_widget_destroy (priv->window);
}

/* Public API */
GcalApplication*
gcal_application_new (void)
{
  g_set_application_name ("Calendar");

  return g_object_new (gcal_application_get_type (),
                       "application-id", "org.gnome.Calendar",
                       "flags", G_APPLICATION_HANDLES_COMMAND_LINE,
                       NULL);
}

GcalManager*
gcal_application_get_manager (GcalApplication *app)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (app);
  return priv->manager;
}

GSettings*
gcal_application_get_settings (GcalApplication *app)
{
  GcalApplicationPrivate *priv;

  priv = gcal_application_get_instance_private (app);
  return priv->settings;
}
