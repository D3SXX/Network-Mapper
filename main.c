// Network-Mapper 0.1 alpha 2 by D3SXX

#include <gtk/gtk.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_ENTRIES 100
#define ENTRY_TEXT_MAX_SIZE 2048

char *netmaskToCIDR(const char *netmask);

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

    // Check if nmap command is available
    FILE *nmap_check = popen("which nmap", "r");
    if (nmap_check == NULL) {
        gtk_text_buffer_insert(buffer, &end, "nmap was not found.\n", -1);
        return;
    }
    pclose(nmap_check);

    // Get local network IP, subnet mask, and broadcast address
    char localIP[256], netmask[256], broadcast[256];
    FILE *ifconfig_output = popen("ifconfig eth0", "r");
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
                    gtk_text_buffer_insert(buffer, &end, " - ", -1);
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


int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Network Mapper");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_widget_set_size_request(window, 600, 400);

    GtkWidget *button_arp = gtk_button_new_with_label("Get ARP Entries");
    GtkWidget *button_nmap = gtk_button_new_with_label("Scan Network");

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(vbox), button_arp, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button_nmap, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    g_signal_connect(button_arp, "clicked", G_CALLBACK(on_button_arp_clicked), text_view);
    g_signal_connect(button_nmap, "clicked", G_CALLBACK(on_button_nmap_clicked), text_view);

    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_widget_show_all(window);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_main();

    return 0;
}
