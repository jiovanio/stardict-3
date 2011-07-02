#include "stardict_espeak.h"
#include <espeak/speak_lib.h>
#include <glib/gi18n.h>
#include <cstring>

static const StarDictPluginSystemInfo *plugin_info = NULL;
static std::string voice_engine;
static IAppDirs* gpAppDirs = NULL;

/* concatenate path1 and path2 inserting a path separator in between if needed. */
static std::string build_path(const std::string& path1, const std::string& path2)
{
	std::string res;
	res.reserve(path1.length() + 1 + path2.length());
	res = path1;
	if(!res.empty() && res[res.length()-1] != G_DIR_SEPARATOR)
		res += G_DIR_SEPARATOR_S;
	if(!path2.empty() && path2[0] == G_DIR_SEPARATOR)
		res.append(path2, 1, std::string::npos);
	else
		res.append(path2);
	return res;
}

static std::string get_cfg_filename()
{
	return build_path(gpAppDirs->get_user_config_dir(), "espeak.cfg");
}

static void saytext(const char *text)
{
	espeak_Synth(text, strlen(text)+1, 0, POS_CHARACTER, 0, espeakCHARS_UTF8, NULL, NULL);
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("Espeak TTS configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	GtkWidget *label = gtk_label_new(_("Voice type:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	GtkWidget *combobox = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), _("Default"));
	gint old_index = 0;
	const espeak_VOICE **voices = espeak_ListVoices(NULL);
	size_t i = 0;
	while (voices[i]) {
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), voices[i]->name);
		if (old_index == 0 && voice_engine == voices[i]->name) {
			old_index = i + 1;
		}
		i++;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), old_index);
	gtk_box_pack_start(GTK_BOX(hbox), combobox, false, false, 0);
	gtk_widget_show_all(hbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), hbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(combobox));
	if (index != old_index) {
		if (index == 0) {
			voice_engine.clear();
		} else {
			voice_engine = voices[index -1]->name;
		}
		if (voice_engine.empty()) {
			espeak_SetVoiceByName("english");
		} else {
			espeak_SetVoiceByName(voice_engine.c_str());
		}
		gchar *data = g_strdup_printf("[espeak]\nvoice=%s\n", voice_engine.c_str());
		std::string res = get_cfg_filename();
		g_file_set_contents(res.c_str(), data, -1, NULL);
		g_free(data);
	}
	gtk_widget_destroy (window);
}

bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading Espeak plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: Espeak plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_TTS;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://www.stardict.org</website></plugin_info>", _("Espeak"), _("Espeak TTS."), _("Pronounce words by Espeak TTS engine."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	gpAppDirs = appDirs;
	return false;
}

void stardict_plugin_exit(void)
{
	espeak_Terminate();
	gpAppDirs = NULL;
}

bool stardict_tts_plugin_init(StarDictTtsPlugInObject *obj)
{
	if(EE_INTERNAL_ERROR == espeak_Initialize(AUDIO_OUTPUT_PLAYBACK, 0, NULL, 0))
		return true;
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[espeak]\nvoice=\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	gchar *str = g_key_file_get_string(keyfile, "espeak", "voice", NULL);
	g_key_file_free(keyfile);
	if (str) {
		voice_engine = str;
		g_free(str);
	}
	if (!voice_engine.empty()) {
		espeak_SetVoiceByName(voice_engine.c_str());
	}
	obj->saytext_func = saytext;
	obj->tts_name = _("Espeak TTS");
	g_print(_("Espeak plug-in loaded.\n"));
	return false;
}
