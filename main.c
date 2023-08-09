// Network-Mapper 0.1 alpha 1 by D3SXX

#include <gtk/gtk.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

void on_button_clicked(GtkWidget *widget, gpointer text_view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    // Execute the netstat command and capture the output
    FILE *netstat_output = popen("netstat -an", "r");
    if (netstat_output) {
        char line[256];
        char clean_line[256];

        while (fgets(line, sizeof(line), netstat_output)) {
            // Remove all non-digit characters (except . and :)
            int j = 0;
            for (int i = 0; i < strlen(line); i++) {
                if (isdigit(line[i]) || line[i] == '.' || line[i] == ':') {
                    clean_line[j] = line[i];
                    j++;
                }
            }
            clean_line[j] = '\0';

            gtk_text_buffer_insert(buffer, &end, clean_line, -1);
            gtk_text_buffer_insert(buffer, &end, "\n", -1);
        }

        pclose(netstat_output);
    } else {
        gtk_text_buffer_insert(buffer, &end, "Failed to execute netstat command.", -1);
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Network Mapper");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 400, 300);

    GtkWidget *button = gtk_button_new_with_label("Get IP list");

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), text_view);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_widget_show_all(window);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_main();

    return 0;
}
