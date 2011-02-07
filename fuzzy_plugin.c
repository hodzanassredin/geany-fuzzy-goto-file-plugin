#include <stdio.h>
#include "geanyplugin.h"

GeanyPlugin         *geany_plugin;
GeanyData           *geany_data;
GeanyFunctions      *geany_functions;

PLUGIN_VERSION_CHECK(147)

PLUGIN_SET_INFO("FuzzyFileSearch", "Fuzzy search for files in project",
                "1.0", "Hodza Nassredin <hodzanassredin@gmail.com>");

static GtkWidget *main_menu_item = NULL;

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata)
{
    //dialogs_show_msgbox(GTK_MESSAGE_INFO, "Fuzzy search");
    //dialogs_show_input("fuzzy search", NULL, "Please type:", "");
    
    //GtkWidget *dialog;

    //dialog = gtk_message_dialog_new(
                              //GTK_WINDOW(geany->main_widgets->window),
                              //GTK_DIALOG_DESTROY_WITH_PARENT,
                              //GTK_MESSAGE_INFO,
                              //GTK_BUTTONS_OK,
                              //_("The following files were dropped:"));
    //gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
                              //"%s", "test");

    //gtk_dialog_run(GTK_DIALOG(dialog));
    //gtk_widget_destroy(dialog);
    GtkWidget *entry, *vbox, *window, * bbox, *scroll, *tree, *okbtn, *cancelbtn;
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	gtk_window_set_title(GTK_WINDOW(window), "Fuzzy search:");
    /* example configuration dialog */
		vbox = gtk_vbox_new(FALSE, 6);
		gtk_container_add(GTK_CONTAINER(window), vbox);
			entry = gtk_entry_new();
			gtk_entry_set_text(GTK_ENTRY(entry), "");
			gtk_container_add(GTK_CONTAINER(vbox), entry);
				scroll = gtk_scrolled_window_new(NULL, NULL);
				gtk_container_add(GTK_CONTAINER(vbox), scroll);
					tree = gtk_tree_view_new();
					gtk_container_add(GTK_CONTAINER(scroll), tree);
			bbox = gtk_hbutton_box_new();
			gtk_container_add(GTK_CONTAINER(vbox), bbox);
				okbtn = gtk_button_new_with_label ("Ok");
				gtk_container_add(GTK_CONTAINER(bbox), okbtn);
				cancelbtn = gtk_button_new_with_label ("Cancel");
				
				gtk_container_add(GTK_CONTAINER(bbox), cancelbtn);
      gtk_widget_show_all(window);
    //gtk_widget_destroy(window);
}

void plugin_init(GeanyData *data)
{
    main_menu_item = gtk_menu_item_new_with_mnemonic("Fuzzy search");
    gtk_widget_show(main_menu_item);
    gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
        main_menu_item);
    g_signal_connect(main_menu_item, "activate",
        G_CALLBACK(item_activate_cb), NULL);
}

void plugin_cleanup(void)
{
    gtk_widget_destroy(main_menu_item);
}

