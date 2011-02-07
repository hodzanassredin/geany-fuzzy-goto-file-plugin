#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "geany.h"
#include "geanyplugin.h"

#ifdef HAVE_GIO
# include <gio/gio.h>
#endif

#define foreach_slist_free(node, list) for (node = list, list = NULL; g_slist_free_1(list), node != NULL; list = node, node = node->next)

GeanyPlugin         *geany_plugin;
GeanyData           *geany_data;
GeanyFunctions      *geany_functions;

PLUGIN_VERSION_CHECK(147)

PLUGIN_SET_INFO("FuzzyFileSearch", "Fuzzy search for files in project",
                "1.0", "Hodza Nassredin <hodzanassredin@gmail.com>");

static GtkWidget *main_menu_item = NULL, *tree = NULL;
static GSList *list = NULL;
static gchar *root_dir;
static GtkCellRenderer     *renderer = NULL;
static 	GtkListStore  *store = NULL;

gint close_dialog( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
  gtk_widget_destroy(widget);
  return(FALSE);
}

gboolean fuzzy_is_ok(const gchar *string1, const gchar *string2)
{
	return header_soundex( string1, string2 ) || strstr( string1, string2 ) != NULL ;
	
	return TRUE;
	return strstr( string1, string2 ) != NULL ;
}

static unsigned char soundex_table[256] = {
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0, 49, 50, 51,  0, 49, 50,  0,  0, 50, 50, 52, 53, 53,  0,
         49, 50, 54, 50, 51,  0, 49,  0, 50,  0, 50,  0,  0,  0,  0,  0,
          0,  0, 49, 50, 51,  0, 49, 50,  0,  0, 50, 50, 52, 53, 53,  0,
         49, 50, 54, 50, 51,  0, 49,  0, 50,  0, 50,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
          0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

void  soundexify(const gchar *sound, gchar code[5])
{
        guchar *c, last = '\0';
        gint n;

        for (c = (guchar *) sound; *c && !isalpha (*c); c++);
        code[0] = toupper (*c);
        memset (code + 1, '0', 3);
        for (n = 1; *c && n < 5; c++) {
                guchar ch = soundex_table[*c];

                if (ch && ch != last) {
                        code[n++] = ch;
                        last = ch;
                }
        }
        code[4] = '\0';
}

//~ gchar *soundex(gchar *word)
//~ {
	//~ char mcode[5];
	//~ soundexify(word, mcode);
	//~ return mcode;
//}

gboolean fuzzy_isslash(char c)
{
	return c == '/';
}

gboolean header_soundex (const char *header, const char *match)
{
        char mcode[5], hcode[5];
        const char *p;
        char c;
        GString *word;
        int truth = FALSE;

        soundexify (match, mcode);

        /* split the header into words, and soundexify and compare each one */
        /* FIXME: Should this convert to utf8, and split based on that, and what not?
           soundex only makes sense for us-ascii though ... */

        word = g_string_new("");
        p = header;
        do {
                c = *p++;
                if (c == 0 || fuzzy_isslash(c)) {
                        if (word->len > 0) {
                                soundexify (word->str, hcode);
                                if (strcmp (hcode, mcode) == 0)
                                        truth = TRUE;
                        }
                        g_string_truncate (word, 0);
                } else if (isalpha (c))
                        g_string_append_c (word, c);
        } while (c && !truth);
        g_string_free (word, TRUE);

        return truth;
}

gint open_file( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;

	/* This will only work in single or browse selection mode! */

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gchar *name;

		gtk_tree_model_get (model, &iter, 0, &name, -1);
		g_print ("selected row is: %s\n", name);
       gchar *path = g_strconcat(root_dir, name, NULL);
		document_open_file(path, FALSE, NULL, NULL);
		
		g_free(name);
	}
	
	gtk_widget_destroy(widget);
	return(FALSE);
}

void view_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
  {
    GtkTreeModel *model;
    GtkTreeIter   iter;

    g_print ("A row has been double-clicked!\n");

    model = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
       gchar *name;

       gtk_tree_model_get(model, &iter, 0, &name, -1);

       g_print ("Double-clicked row contains name %s\n", name);
       gchar *path = g_strconcat(root_dir, name, NULL);
		document_open_file(path, FALSE, NULL, NULL);
       g_free(name);
    }
  }

void fuzzy_init()
{

}

void fuzzy_add_dir(gchar *dir, const gchar *needle, GtkTreeIter iter)
{
	gboolean is_dir;
    gchar *utf8_name;
    gchar *fname;
    gchar *subname;    
	gchar *uri;
	GSList *node;
	gchar *path = g_strconcat(root_dir, dir, NULL);
	list  =  utils_get_file_list(path, NULL, NULL);
	if (list != NULL)
	{
		foreach_slist_free(node, list)
		{
			fname 		= g_strconcat("/", node->data, NULL);
			uri 		= g_strconcat(dir, fname, NULL);
			gchar *newpath = g_strconcat(root_dir, uri, NULL);
			is_dir 		= g_file_test (newpath, G_FILE_TEST_IS_DIR);
			utf8_name 	= utils_get_utf8_from_locale(fname);
			if (!is_dir)
			{
				if(fuzzy_is_ok( uri, needle ))
				{
					gtk_list_store_append (store, &iter);
					gtk_list_store_set (store, &iter, 0, uri, -1);	
				}
			}
			else
			{
				fuzzy_add_dir(uri, needle, iter);
			}
			g_free(utf8_name);
			g_free(uri);
			g_free(fname);
    	}
	}
}

void fuzzy_populate_files(const gchar *needle)
{
	GtkTreeIter    iter;
                                   
	GeanyProject 	*project 	= geany->app->project;
	GeanyDocument	*doc 		= document_get_current();
	
	if (project)
		root_dir = project->base_path;
	else
		root_dir = geany->prefs->default_open_path;

	GtkTreeViewColumn *oldCol = gtk_tree_view_get_column(GTK_TREE_VIEW (tree), 0);
	if (oldCol != NULL)
	{
		gtk_tree_view_column_clear(oldCol);
		gtk_tree_view_remove_column(GTK_TREE_VIEW (tree), GTK_TREE_VIEW_COLUMN (oldCol));
	}
	
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tree),
                                               -1,      
                                               "Name",  
                                               renderer,
                                               "text", 0,
                                               NULL);
	//gtk-tree-view-column-clear
	//gtk_tree_view_remove_column  
     store = gtk_list_store_new (1, G_TYPE_STRING);
	
    fuzzy_add_dir("", needle, iter);
	gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));
}



static gboolean cb_entry_changed( GtkEditable *entry, GtkTreeView *treeview )
{
    //~ GtkTreeModelFilter *filter;
   //~ 
    //~ filter = GTK_TREE_MODEL_FILTER( gtk_tree_view_get_model( treeview ) );
    //~ gtk_tree_model_filter_refilter( filter );
    const  gchar *text = gtk_entry_get_text( GTK_ENTRY(entry));
    fuzzy_populate_files(text);
    return( FALSE );
} 

//~ static gboolean filter_func( GtkTreeModel *model, GtkTreeIter  *iter, GtkEntry     *entry )
//~ {
    //~ const gchar *needle;
    //~ gchar       *haystack;
   //~ 
    //~ gtk_tree_model_get( model, iter, COL_TEXT, &haystack, -1 );
    //~ needle = gtk_entry_get_text( entry );
   //~ 
    //~ if( strstr( haystack, needle ) != NULL )
        //~ return( TRUE );
    //~ else
        //~ return( FALSE );
//~ } 

static void item_activate_cb(GtkMenuItem *menuitem, gpointer gdata)
{
	fuzzy_init();
    GtkWidget *entry, *vbox, *window, * bbox, *scroll, *okbtn, *cancelbtn;
	
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	gtk_window_set_default_size(GTK_WINDOW(window), 500, 300);
	gtk_window_set_title(GTK_WINDOW(window), "Fuzzy search:");
    /* example configuration dialog */
		vbox = gtk_vbox_new(FALSE, 6);
		gtk_container_add(GTK_CONTAINER(window), vbox);
			entry = gtk_entry_new();
			g_signal_connect( G_OBJECT( entry ), "changed",
                      G_CALLBACK( cb_entry_changed ),
                      GTK_TREE_VIEW( tree ) ); 
			gtk_entry_set_text(GTK_ENTRY(entry), "");
			gtk_container_add(GTK_CONTAINER(vbox), entry);
				scroll = gtk_scrolled_window_new(NULL, NULL);
				gtk_container_add(GTK_CONTAINER(vbox), scroll);
					tree = gtk_tree_view_new();
					g_signal_connect(tree, "row-activated", (GCallback) view_onRowActivated, NULL);
					fuzzy_populate_files("/");
					gtk_container_add(GTK_CONTAINER(scroll), tree);
			bbox = gtk_hbutton_box_new();
			gtk_container_add(GTK_CONTAINER(vbox), bbox);
				okbtn = gtk_button_new_with_label ("Ok");
				gtk_container_add(GTK_CONTAINER(bbox), okbtn);
				gtk_signal_connect_object (GTK_OBJECT (okbtn), "clicked",
                               GTK_SIGNAL_FUNC(open_file),
                               GTK_OBJECT (window));
				cancelbtn = gtk_button_new_with_label ("Cancel");
				gtk_signal_connect_object (GTK_OBJECT (cancelbtn), "clicked",
                               GTK_SIGNAL_FUNC(close_dialog),
                               GTK_OBJECT (window));
				gtk_container_add(GTK_CONTAINER(bbox), cancelbtn);
      gtk_widget_show_all(window);
}

static void kb_activate(guint key_id)
{
	//gtk_notebook_set_current_page(GTK_NOTEBOOK(geany->main_widgets->sidebar_notebook), page_number);
	switch (key_id)
	{
		case 0:
			item_activate_cb(NULL, NULL);
			break;
	}
}

void plugin_init(GeanyData *data)
{
	GeanyKeyGroup *key_group;
	
    main_menu_item = gtk_menu_item_new_with_mnemonic("Fuzzy search");
    gtk_widget_show(main_menu_item);
    gtk_container_add(GTK_CONTAINER(geany->main_widgets->tools_menu),
        main_menu_item);
    g_signal_connect(main_menu_item, "activate",
        G_CALLBACK(item_activate_cb), NULL);
        
    	/* setup keybindings */
	key_group = plugin_set_key_group(geany_plugin, "fuzzy_search", 1, NULL);

	keybindings_set_item(key_group, 0, kb_activate,
		0, 0, "fuzzy_search_open", _("fuzzy search open"), NULL);
}

void plugin_cleanup(void)
{
    gtk_widget_destroy(main_menu_item);
}

