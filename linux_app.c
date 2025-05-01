#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pango/pango.h>

#include "core_logic.h"

typedef struct {
    GtkWidget *keybase_entry;
    GtkWidget *keyinc_entry;
    GtkTextView *text_view;
    GtkWidget *status_bar;
} AppWidgets;

AppWidgets widgets;
guint status_context_id;

char* get_text_from_textview(GtkTextView *text_view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    return gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
}

void set_text_in_textview(GtkTextView *text_view, const char *text) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(text_view);
    gtk_text_buffer_set_text(buffer, text, -1);
}

void show_status_message(const char *message) {
    gtk_statusbar_push(GTK_STATUSBAR(widgets.status_bar), status_context_id, message);
}

void on_encode_button_clicked(GtkWidget *widget, gpointer data) {
    const char *keybase_str = gtk_entry_get_text(GTK_ENTRY(widgets.keybase_entry));
    const char *keyinc_str = gtk_entry_get_text(GTK_ENTRY(widgets.keyinc_entry));

    long keybase, keyinc;
    char *endptr_base, *endptr_inc;
    int keys_valid = 1;

    show_status_message("");

    keybase = strtol(keybase_str, &endptr_base, 10);
    keyinc = strtol(keyinc_str, &endptr_inc, 10);

    if (*keybase_str == '\0' || *endptr_base != '\0') {
        show_status_message("Error: keybase value must be an integer.");
        keys_valid = 0;
    }
    if (*keyinc_str == '\0' || *endptr_inc != '\0') {
        show_status_message("Error: keyinc value must be an integer.");
        keys_valid = 0;
    }

    if (!keys_valid) {
        return;
    }

    if (keybase < 0 || keybase >= BASE64_LEN || keyinc < 0 || keyinc >= BASE64_LEN) {
        char msg[100];
        sprintf(msg, "Error: Keys must be 0-%d.", BASE64_LEN - 1);
        show_status_message(msg);
        return;
    }

    char *input_text_original = get_text_from_textview(widgets.text_view);
    if (!input_text_original) {
         show_status_message("Error getting text from input area.");
         return;
    }

    char *input_text_normalized = normalize_crlf(input_text_original);
    g_free(input_text_original);

    if (!input_text_normalized) {
         show_status_message("Memory allocation failed during CRLF normalization.");
         return;
    }

    char *error_msg = NULL;
    char *output_text = encode_and_encrypt_string(input_text_normalized, keybase, keyinc, &error_msg);

    free(input_text_normalized);

    if (output_text) {
        set_text_in_textview(widgets.text_view, output_text);
        free(output_text);
        show_status_message("Encoding successful.");
    } else {
        show_status_message(error_msg ? error_msg : "An unknown error occurred during encoding.");
         if (error_msg) free(error_msg);
    }
}

int extract_keys_from_header(const char* text, int* keybase, int* keyinc) {
    if (!text) return 0;
    
    if (sscanf(text, "KEYBASE:%d KEYINC:%d", keybase, keyinc) == 2) {
        if (*keybase >= 0 && *keybase < BASE64_LEN && *keyinc >= 0 && *keyinc < BASE64_LEN) {
            return 1;
        }
    }
    return 0;
}

void on_decode_button_clicked(GtkWidget *widget, gpointer data) {
    char *input_text = get_text_from_textview(widgets.text_view);
    if (!input_text) {
        show_status_message("Error getting text from input area.");
        return;
    }

    int auto_keys = 0;
    int keybase = 0, keyinc = 0;
    
    if (*input_text != '\0') {
        if (extract_keys_from_header(input_text, &keybase, &keyinc)) {
            char keybase_str[10], keyinc_str[10];
            snprintf(keybase_str, sizeof(keybase_str), "%d", keybase);
            snprintf(keyinc_str, sizeof(keyinc_str), "%d", keyinc);
            gtk_entry_set_text(GTK_ENTRY(widgets.keybase_entry), keybase_str);
            gtk_entry_set_text(GTK_ENTRY(widgets.keyinc_entry), keyinc_str);
            auto_keys = 1;
        }
    }

    const char *keybase_str = gtk_entry_get_text(GTK_ENTRY(widgets.keybase_entry));
    const char *keyinc_str = gtk_entry_get_text(GTK_ENTRY(widgets.keyinc_entry));

    long keybase_manual, keyinc_manual;
    char *endptr_base, *endptr_inc;
    int keys_valid = 1;

    show_status_message("");

    keybase_manual = strtol(keybase_str, &endptr_base, 10);
    keyinc_manual = strtol(keyinc_str, &endptr_inc, 10);

    if (*keybase_str == '\0' || *endptr_base != '\0') {
        show_status_message("Error: keybase value must be an integer.");
        keys_valid = 0;
    }
    if (*keyinc_str == '\0' || *endptr_inc != '\0') {
        show_status_message("Error: keyinc value must be an integer.");
        keys_valid = 0;
    }

    if (!keys_valid) {
        g_free(input_text);
        return;
    }

    if (keybase_manual < 0 || keybase_manual >= BASE64_LEN || 
        keyinc_manual < 0 || keyinc_manual >= BASE64_LEN) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Error: Keys must be 0-%d.", BASE64_LEN - 1);
        show_status_message(msg);
        g_free(input_text);
        return;
    }

    if (auto_keys) {
        keybase_manual = keybase;
        keyinc_manual = keyinc;
    }

    char *error_msg = NULL;
    char *output_text = decrypt_and_decode_string(input_text, keybase_manual, keyinc_manual, &error_msg);

    g_free(input_text);

    if (output_text) {
        set_text_in_textview(widgets.text_view, output_text);
        free(output_text);
        show_status_message("Decoding successful.");
    } else {
        show_status_message(error_msg ? error_msg : "An unknown error occurred during decoding.");
        if (error_msg) free(error_msg);
    }
}

void on_copy_button_clicked(GtkWidget *widget, gpointer data) {
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    char *text_to_copy = get_text_from_textview(widgets.text_view);

    if (text_to_copy && *text_to_copy != '\0') {
        gtk_clipboard_set_text(clipboard, text_to_copy, -1);
        show_status_message("Text copied to clipboard.");
    } else {
        show_status_message("Text area is empty. Nothing to copy.");
    }
    if (text_to_copy) g_free(text_to_copy);
}

void paste_received_cb(GtkClipboard *clipboard, const gchar *text, gpointer data) {
    if (text && *text != '\0') {
        set_text_in_textview(widgets.text_view, text);
        show_status_message("Text pasted from clipboard.");
    } else {
         show_status_message("Clipboard does not contain plain text.");
    }
}

void on_paste_button_clicked(GtkWidget *widget, gpointer data) {
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    show_status_message("");
    gtk_clipboard_request_text(clipboard, paste_received_cb, NULL);
}

void on_clear_canvas_clicked(GtkWidget *widget, gpointer data) {
    set_text_in_textview(widgets.text_view, "");
    show_status_message("Text area cleared.");
}

void on_clear_clipboard_clicked(GtkWidget *widget, gpointer data) {
    GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(clipboard, "", -1);
    show_status_message("Clipboard cleared.");
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *hbox_keys;
    GtkWidget *hbox_buttons;
    GtkWidget *label_base, *label_inc;
    GtkWidget *scrolled_window;
    GtkWidget *encode_button, *decode_button, *copy_button, *paste_button;
    GtkWidget *clear_canvas_button, *clear_clipboard_button;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Yet Another SCOS");
    gtk_window_set_default_size(GTK_WINDOW(window), 750, 400);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    hbox_keys = gtk_hbox_new(FALSE, 5);
    label_base = gtk_label_new("keybase:");
    widgets.keybase_entry = gtk_entry_new();
    label_inc = gtk_label_new("keyinc:");
    widgets.keyinc_entry = gtk_entry_new();

    gtk_entry_set_max_length(GTK_ENTRY(widgets.keybase_entry), 2);
    gtk_entry_set_max_length(GTK_ENTRY(widgets.keyinc_entry), 2);

    gtk_widget_set_size_request(widgets.keybase_entry, 45, -1);
    gtk_widget_set_size_request(widgets.keyinc_entry, 45, -1);

    gtk_box_pack_start(GTK_BOX(hbox_keys), label_base, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_keys), widgets.keybase_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_keys), label_inc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_keys), widgets.keyinc_entry, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox_keys, FALSE, FALSE, 0);

    scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_IN);

    widgets.text_view = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_wrap_mode(widgets.text_view, GTK_WRAP_WORD);

    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace");
    if (font_desc) {
         gtk_widget_modify_font(GTK_WIDGET(widgets.text_view), font_desc);
         pango_font_description_free(font_desc);
    }

    gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(widgets.text_view));

    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

    hbox_buttons = gtk_hbox_new(FALSE, 5);
    encode_button = gtk_button_new_with_label("Encode");
    decode_button = gtk_button_new_with_label("Decode");
    copy_button = gtk_button_new_with_label("Copy");
    paste_button = gtk_button_new_with_label("Paste");
    clear_canvas_button = gtk_button_new_with_label("Clear Canvas");
    clear_clipboard_button = gtk_button_new_with_label("Clear Clipboard");

    g_signal_connect(encode_button, "clicked", G_CALLBACK(on_encode_button_clicked), NULL);
    g_signal_connect(decode_button, "clicked", G_CALLBACK(on_decode_button_clicked), NULL);
    g_signal_connect(copy_button, "clicked", G_CALLBACK(on_copy_button_clicked), NULL);
    g_signal_connect(paste_button, "clicked", G_CALLBACK(on_paste_button_clicked), NULL);
    g_signal_connect(clear_canvas_button, "clicked", G_CALLBACK(on_clear_canvas_clicked), NULL);
    g_signal_connect(clear_clipboard_button, "clicked", G_CALLBACK(on_clear_clipboard_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(hbox_buttons), encode_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), decode_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), copy_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), paste_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), clear_canvas_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_buttons), clear_clipboard_button, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), hbox_buttons, FALSE, FALSE, 0);

    widgets.status_bar = gtk_statusbar_new();
    status_context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(widgets.status_bar), "app_status");
    gtk_box_pack_end(GTK_BOX(vbox), widgets.status_bar, FALSE, FALSE, 0);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}