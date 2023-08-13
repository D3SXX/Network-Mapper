// Network-Mapper 0.1 alpha 4 by D3SXX

#include <gtk/gtk.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_ENTRIES 100
#define ENTRY_TEXT_MAX_SIZE 2048

char *netmaskToCIDR(const char *netmask);

gboolean is_moving_block = FALSE;
int selected_block_index = -1;
double selected_block_offset_x = 0.0;
double selected_block_offset_y = 0.0;

typedef struct {
    char localName[256];
    char ipAddress[256];
    char macAddress[256];
    char ethernetInterface[256];
} ArpEntry;

ArpEntry entries[MAX_ENTRIES];
int numEntries = 0;

void clear_text_view(GtkTextBuffer *buffer) {
    gtk_text_buffer_set_text(buffer, "", -1);
}

void parse_arp_output(const char *line) {
    if (numEntries >= MAX_ENTRIES) {
        return; // To prevent array overflow
    }
	// debian
    sscanf(line, "%255s (%255[^)]) at %255s [ether] on %255s", 
           entries[numEntries].localName, 
           entries[numEntries].ipAddress, 
           entries[numEntries].macAddress, 
           entries[numEntries].ethernetInterface);
    
    numEntries++;
}

void on_button_arp_clicked(GtkWidget *widget, gpointer text_view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    clear_text_view(buffer);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    // Execute the arp command and capture the output
    FILE *arp_output = popen("arp -a", "r");
    if (arp_output) {
        char line[256];

        while (fgets(line, sizeof(line), arp_output)) {
            // Check if the line contains a sequence that resembles an IP address
            if (strstr(line, ":") && strstr(line, ".")) {
                parse_arp_output(line);
            }
        }

        pclose(arp_output);

    for (int i = 0; i < numEntries; i++) {
        // Insert information into the text view
        char entryText[ENTRY_TEXT_MAX_SIZE];
        snprintf(entryText, sizeof(entryText), 
                 "Local Name: %s\nIP Address: %s\nMAC Address: %s\nEthernet Interface: %s\n\n",
                 entries[i].localName, entries[i].ipAddress, entries[i].macAddress, entries[i].ethernetInterface);
        gtk_text_buffer_insert(buffer, &end, entryText, -1);
    }
    } else {
        gtk_text_buffer_insert(buffer, &end, "Failed to execute arp command.", -1);
    }
}

void on_button_nmap_clicked(GtkWidget *widget, gpointer text_view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    clear_text_view(buffer);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    if (entries[0].ethernetInterface[0] == '\0'){
        gtk_text_buffer_insert(buffer, &end, "Please press 'Get ARP Entries'.\n", -1);
        return;
    }

    // Check if nmap command is available
    FILE *nmap_check = popen("which nmap", "r");
    if (nmap_check == NULL) {
        gtk_text_buffer_insert(buffer, &end, "nmap was not found, please install it.\n", -1);
        return;
    }
    pclose(nmap_check);

    // Get local network IP, subnet mask, and broadcast address
    char localIP[256], netmask[256], broadcast[256];
    char command[256];
    snprintf(command, sizeof(command), "ifconfig %s", entries[0].ethernetInterface);
    FILE *ifconfig_output = popen(command, "r");
    if (ifconfig_output) {
        char line[512];
        while (fgets(line, sizeof(line), ifconfig_output)) {
            if (strstr(line, "inet ")) {
                sscanf(line, "        inet %255s  netmask %255s  broadcast %255s", localIP, netmask, broadcast);
            }
        }
        pclose(ifconfig_output);
    }

    // Construct the nmap command with calculated values
    char nmapCommand[512];
    snprintf(nmapCommand, sizeof(nmapCommand), "nmap -sP %s/%s", localIP, netmaskToCIDR(netmask)); // Use function to convert netmask to CIDR
    char nmap_string[256];
    snprintf(nmap_string, sizeof(nmap_string), "Executing nmap for ip address %s/%s.\n", localIP, netmaskToCIDR(netmask));
    gtk_text_buffer_insert(buffer, &end, nmap_string, -1);

    // Execute the nmap command and capture the output
    FILE *nmap_output = popen(nmapCommand, "r");
    if (nmap_output) {
        char line[256];
        char summary[256] = "";

        while (fgets(line, sizeof(line), nmap_output)) {
            // Extract the summary sentence from Nmap output
            if (strstr(line, "Nmap done: ")) {
                strncpy(summary, line, sizeof(summary) - 1);
                summary[sizeof(summary) - 1] = '\0';
            }

            // Extract local names and IP addresses from the Nmap output
            if (strstr(line, "Nmap scan report for ")) {
                char localName[256], ipAddress[256];
                if (sscanf(line, "Nmap scan report for %255[^ ] %255[^\n]", localName, ipAddress) == 2) {
                    gtk_text_buffer_insert(buffer, &end, localName, -1);
                    gtk_text_buffer_insert(buffer, &end, " ", -1);
                    gtk_text_buffer_insert(buffer, &end, ipAddress, -1);
                    gtk_text_buffer_insert(buffer, &end, "\n", -1);
                }
            }
        }

        // Display the summary sentence
        if (strlen(summary) > 0) {
            gtk_text_buffer_insert(buffer, &end, summary, -1);
        } else {
            gtk_text_buffer_insert(buffer, &end, "Nmap summary not found.\n", -1);
        }

        pclose(nmap_output);
    } else {
        gtk_text_buffer_insert(buffer, &end, "Failed to execute nmap command.", -1);
    }
}


// Function to convert netmask to CIDR notation
char *netmaskToCIDR(const char *netmask) {
    int mask[4];
    sscanf(netmask, "%d.%d.%d.%d", &mask[0], &mask[1], &mask[2], &mask[3]);

    int cidr = 0;
    for (int i = 0; i < 4; i++) {
        cidr += __builtin_popcount(mask[i]);
    }

    static char cidrStr[4];
    snprintf(cidrStr, sizeof(cidrStr), "%d", cidr);
    return cidrStr;
}

gboolean on_drawing_area_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    // Clear the drawing area
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // Set background color to white
    cairo_paint(cr);

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    int rectangle_x = 85;
    int rectangle_y = 40;

    int x = 0;
    int y = 0;

    // Draw objects based on entries
    for (int i = 0; i < numEntries; i++) {

        if (x >= width-200) {
            x = 100;
            y += 100;

        }
        else{
            x += 100;
        }


        // Debug
        printf("Block %d size: %dx%d\n",i+1,rectangle_x,rectangle_y);
        printf("width: %d height: %d X: %d Y: %d\n",width,height,x,y);

        // Draw a block
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0); // Set block color to black
        cairo_rectangle(cr, x, y, rectangle_x, rectangle_y); // Adjust the size and position as needed
        cairo_fill(cr);

        // Draw text inside the block
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0); // Set text color to white
        cairo_move_to(cr, x , y + rectangle_y * 0.25); // Adjust the position for text
        cairo_show_text(cr, entries[i].localName); // Draw localName
        cairo_move_to(cr, x ,y + rectangle_y * 0.5 );
        cairo_show_text(cr, entries[i].ipAddress); // Draw IP address
        cairo_move_to(cr, x ,y + rectangle_y * 0.85 );
        cairo_show_text(cr, entries[i].macAddress); // Draw Mac Address
    }

    return FALSE; // Let the default drawing handler do the rest
}

void on_button_open_map_clicked(GtkWidget *widget, gpointer user_data) {
    GtkWidget *map_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(map_window), "Network Map");
    gtk_container_set_border_width(GTK_CONTAINER(map_window), 10);
    gtk_widget_set_size_request(map_window, 800, 600);

    // Create a drawing area for the network map
    GtkWidget *drawing_area = gtk_drawing_area_new();

    // Connect the draw signal to the drawing function
    g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_drawing_area_draw), NULL);

    // Add the drawing area to the map window
    gtk_container_add(GTK_CONTAINER(map_window), drawing_area);

    // Show the network map window
    gtk_widget_show_all(map_window);
}


int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Network Mapper");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 600, 400);

    GtkWidget *button_arp = gtk_button_new_with_label("Get ARP Entries");
    GtkWidget *button_nmap = gtk_button_new_with_label("Scan Network");
    GtkWidget *button_open_map = gtk_button_new_with_label("Open Network Map");

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_arp, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button_nmap, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button_open_map, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    g_signal_connect(button_arp, "clicked", G_CALLBACK(on_button_arp_clicked), text_view);
    g_signal_connect(button_nmap, "clicked", G_CALLBACK(on_button_nmap_clicked), text_view);
    g_signal_connect(button_open_map, "clicked", G_CALLBACK(on_button_open_map_clicked), NULL);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_widget_show_all(window);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_main();

    return 0;
}
