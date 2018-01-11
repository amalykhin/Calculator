#include <gtk/gtk.h>
#include <assert.h>

#ifdef DEBUG
	#define DEBUG_FUNCTION(NAME) g_printerr("%s\n", (NAME))
#else
	#define DEBUG_FUNCTION(NAME) {} 
#endif

enum Operation {
	OP_ADD       =  '+',
	OP_DIVIDE    =  '/',
	OP_MULTIPLY  =  '*',
	OP_PERCENT   =  '%',
	OP_SUBTRACT  =  '-',
	OP_EQUALS    =  '=',
};

struct Calculator {
	double reg_a;
	double reg_b;
	enum Operation op;
	//Basically, op2 is used only when "op" is OP_EQUALS
	//to reexecute the old operation on consecutive = press.
	enum Operation op2;
} calculator;

gboolean reset_panel = FALSE;

void    button_misc_handler       (GtkWidget  *widget,  gpointer  txt_panel);
void    button_number_handler     (GtkWidget  *widget,  gpointer  txt_panel);
void    button_operation_handler  (GtkWidget  *widget,  gpointer  txt_panel);
void    destroy_callback          (GtkWidget  *widget,  gpointer  data);
double  calculator_add            (double     a,        double    b);
double  calculator_divide         (double     a,        double    b);
double  calculator_multiply       (double     a,        double    b);
double  calculator_percent        (double     a,        double    b);
double  calculator_subtract       (double     a,        double    b);
void	calculator_reset	  (struct Calculator *calc);
void    swap                      (double    *a,        double   *b);


int
main (int argc, char **argv)
{
	GtkWidget *window;
	GtkBuilder *builder;

	gtk_init (&argc, &argv);

	calculator_reset(&calculator);
	builder = gtk_builder_new_from_file ("/home/tigr/calculator/calculator.glade");
	window = GTK_WIDGET(gtk_builder_get_object (builder, "win_main"));

	gtk_builder_connect_signals (builder, NULL);
	g_signal_connect (window, "destroy", 
			G_CALLBACK (destroy_callback), NULL);

	gtk_window_set_focus_visible (GTK_WINDOW(window), FALSE);
	gtk_widget_show_all (GTK_WIDGET (window));

	gtk_main ();

	return 0;
}

void 
destroy_callback (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

G_MODULE_EXPORT void 
button_number_handler (GtkWidget *widget, gpointer txt_panel)
{
	GtkEntryBuffer *buffer;
	guint len;
	const gchar *label;

	DEBUG_FUNCTION("button_number_handler");

	buffer = gtk_entry_get_buffer (GTK_ENTRY (txt_panel));

	len   = gtk_entry_buffer_get_length(buffer);
	label = gtk_button_get_label(GTK_BUTTON(widget));
	if (reset_panel) {
		gtk_entry_buffer_delete_text(buffer, 0, -1);
		reset_panel = FALSE;
	}
	if (gtk_entry_buffer_get_text(buffer)[0] == '0'
		&& gtk_entry_buffer_get_text(buffer)[1] != '.') 
		return;
	gtk_entry_buffer_insert_text (buffer, len, label, -1);
}

G_MODULE_EXPORT void 
button_misc_handler (GtkWidget *widget, gpointer txt_panel)
{
	GtkEntryBuffer *buffer;
	const gchar *label;
	guint len;
	static gboolean is_integer = TRUE;

	DEBUG_FUNCTION("button_misc_handler");

	buffer = gtk_entry_get_buffer (GTK_ENTRY (txt_panel));
	len = gtk_entry_buffer_get_length(buffer);

	if ((label = gtk_button_get_label(GTK_BUTTON(widget)))[0] == 'C') {
		gtk_entry_buffer_delete_text (buffer, 0, -1);
		is_integer = TRUE;
		calculator_reset(&calculator);
	} else if (label[0] == '.' && is_integer) {
		label = len ? label : "0.";
		gtk_entry_buffer_insert_text(buffer, len, label, -1);
		is_integer = FALSE;
	} else if (len > 0 && g_utf8_get_char(label) == 0x232b) {
		is_integer = gtk_entry_buffer_get_text(buffer)[len-1] == '.' ? TRUE : is_integer;
		gtk_entry_buffer_delete_text(buffer, len-1, 1); 
	}

}

double calculator_add (double a, double b)
{
	return a+b;
}

double calculator_subtract (double a, double b)
{
	return a-b;
}

double calculator_multiply (double a, double b)
{
	return a*b;
}

double calculator_divide (double a, double b)
{
	if (b)
		return a/b;
}

double calculator_percent (double a, double b)
{
	return b/100*a;
}

void button_operation_handler (GtkWidget *widget, gpointer txt_panel)
{
	const gchar *label, *text;
	GtkEntryBuffer *buffer;
	GString *str;
	gdouble a, b;
	enum Operation *op;

	DEBUG_FUNCTION("button_operation_handler");

	label  = gtk_button_get_label(GTK_BUTTON(widget));
	buffer = gtk_entry_get_buffer(GTK_ENTRY(txt_panel));
	text   = gtk_entry_buffer_get_text(buffer);

	reset_panel = TRUE;
	//If reg_b is 0 initialize it.
	if (!calculator.op)
		calculator.reg_b = g_strtod(text, NULL);
	//If handler was called by consecutive presses of =, then don't 
	//rewrite reg_a.
	else if (calculator.op != OP_EQUALS) 
		calculator.reg_a = g_strtod(text, NULL);

	a = calculator.reg_a;
	b = calculator.reg_b;
	//If = was pressed, then it is writen to calculator.op.
	//But, if = is pressed couple times in a row, execute the operation in
	//calculator.op2 instead of calculator.op.
	//Also, if after = another operation is chosen, handle it properly:
	//don't execute the old operation, and just write the new operation.
	op = calculator.op == OP_EQUALS && label[0]=='='? &calculator.op2 : &calculator.op;
	//if (!(label[0] != '=' && calculator.op == OP_EQUALS))
	if (label[0] == '%' || calculator.op == OP_EQUALS && calculator.op2 == OP_PERCENT) 
		a = calculator_percent(b, a);
	
	if (*op == OP_ADD)
		calculator.reg_b = calculator_add(a, b);
	else if (*op == OP_SUBTRACT)
		calculator.reg_b = calculator_subtract(b, a);
	else if (*op == OP_MULTIPLY)
		calculator.reg_b = calculator_multiply(a, b);
	else if (*op == OP_DIVIDE)
		calculator.reg_b = calculator_divide(b, a);
	g_string_printf(str = g_string_new(NULL), 
			"%g", 
			calculator.reg_b);
	gtk_entry_buffer_set_text(buffer, str->str, -1);

	//Writes new operations until a = operation is written.
	//Then, write new operation, if it is not = operation.
	if (label[0] != '=' || calculator.op != OP_EQUALS) {
		calculator.op2 = calculator.op;
		calculator.op    = label[0];
	}
}

void swap (double *a, double *b)
{
	double aux;

	aux = *a;
	*a = *b;
	*b = aux;
}

void calculator_reset (struct Calculator *calc)
{
	calc->reg_a = 0;
	calc->reg_b = 0;
	calc->op = 0;
	calc->op2 = 0;
}
