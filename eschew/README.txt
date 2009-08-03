                             
                           ESCHEW - Even Simpler C-Heuristics Expat Wrapper
                           
                           USAGE NOTES


	* GOAL:
	This is a XML simple DOM tables constructor + runtime parser wrapper
	The goal here is to create XML node tables that are available at compilation 
	time (so that they can be referenced statically in the program) and provide
	the helper classes to populate these nodes from an XML file at runtime.
	Eschew is typically used to set application options from an XML conf file.

	* ESCHEW vs SCEW, or why not use SCEW (Simple C Expat Wrapper) instead?
	1. Scew's node values are string only, whereas Eschew can define and populate 
	   any type
	2. Scew's nodes are only available at runtime => massive overhead everytime you
	   access a simple value. If you define user input controls in the XML, this is
	   not a viable option. In Eschew on the other hand, table values are *directly*
	   accessible at compilation time, in their declared type, whilst still being 
	   populated from the XML at runtime
	3. Scew provide writing to XML, mutliple attribute handling and other features
	   which are not really required for a simple xml options readout operation
	3. As the nodes you define are automatically added as enums, you must make sure
	   that your node names don't conflict with keywords or variable names

	Once again, the main advantage of Eschew is that, after the XML file has been 
	read, NO string	or cycle consuming resolution intervenes to access the table 
	values. They are available like any other regular C array of values (with the
	node names defined as enum for you to facilitate access). 
	For instance, if you use the Eschew macros to create the xml_keys_player_1 
	XML table (which includes the node "dir_up"), you can directly use in your 
	source:

		if (current_key = xml_keys_player_1.value[dir_up])
			move_player_forward();

	* FEATURES
	- Read and write XML configuration files (Re-write of XML conf possible)
	- Preservation/Creation of comments

	* LIMITATIONS
	The following limitations are in effect when using this XML wrapper:
	- Only one atribute can be declared per node, which is used to differentiate 
	  between nodes of the same name. See example below for the "keys" node
	- Child nodes of a specific branch must all be of the same type (because they
	  all belong to the same C table, which of course can only have one type in C)
	  However, if you declare an integer xml tablein Eschew, boolean values like
	  "true", "enabled", "on", "false", "disable", "off" will be detected and set 
	  accordingly (i.e. false -> 0, true -> non zero)
	- Node definitions must follow the logical order, i.e. parent nodes must be 
	  instantiated before of their children, for the macros to work.
	- You cannot use underscores in your base table names, as underscores are used
	  to separate attributes properties (but if you feel this is too much of a 
	  limitation, you can change the XML_ATTRIBUTE_SEPARATOR definition to set the
	  attribute separator to something else). You are free to use underscore in
	  node names.
	- You must use one of the xml_#### types when defining your table. If you look
	  at the .h, you will see that there are just a redefinition for what you'd 
	  expect (eg: xml_unsigned_char is a redefintion of "unsigned char"). The 
	  reason is that any type used for your defintions must be a single word.

	* EXAMPLE
	Suppose you want to make the following configuration options available to your 
	program:

	<?xml version="1.0" encoding="utf-8"?>
	<config>
	  <resolution>
	    <scr_width>1280</scr_width>
		<scr_height>1024</scr_height>
		<fullscreen>disabled</fullscreen>
	  </resolution>
	  <keys player="1">
	    <dir_left>&#x8C;</dir_left>
		<dir_right>&#x8E;</dir_right>
		<dir_up>&#x8D;</dir_up>
		<dir_down>&#x8F;</dir_down>
	  </keys>
	  <keys player="2">
	    <left>a</left>
		<right>d</right>
		<up>w</up>
		<down>s</down>
	  </keys>
	  <messages>
	    <msg1>ha ha! pwnd!</msg1>
	    <msg2>%s has left the game</msg2>
	  </messages>
	</config>

	With Eschew, you simply need to create a header with the following:

	DEFINE_XML_NODES(config_nodes, resolution, keys, messages)
	// Table ot tables => xml_node type
	CREATE_XML_TABLE(config, config_nodes, xml_node) 
	// config is the root node => declare it as such
	SET_XML_ROOT(config)

	DEFINE_XML_NODES(res_nodes, scr_width, scr_height, fullscreen)
	// Integers or booleans => xml_int
	CREATE_XML_TABLE(resolution, res_nodes, xml_int)

	DEFINE_XML_NODES(keys_nodes, dir_left, dir_right, dir_up, dir_down)
	// Single character values => xml_unsigned_char
	CREATE_XML_TABLE(keys_player_1, keys_nodes, xml_unsigned_char)
	CREATE_XML_TABLE(keys_player_2, keys_nodes, xml_unsigned_char)

	DEFINE_XML_NODES(msg_nodes, msg1, msg2)
	// Strings => xml_string
	CREATE_XML_TABLE(messages, msg_nodes, xml_string)


	This results in the creation of the following tables which you can
	use	straight away (with the content initialized at runtime)

	int				resolution.value[3] = {1280, 1024, 0};
	unsigned char	keys_player_1.value[4] = {0x8C, 0x8E, 0x8D, 0x8F};
	unsigned char	keys_player_2.value[4] = {'a', 'd', 'w', 's'};
	char*			messages.values[2] = { "ha ha! pwnd!", "%s has left the game"};

	The following enums are also automatically set

	enum { resolution, keys_player_1, keys_player_2, messages };
	enum { dir_left, dir_right, dir_up, dir_down };
	enum { msg1, msg2 };

	If needed, the names for the nodes are also available in 
	xml_<table_name>.name[i], as well as the parent table (xml_config)


	Then in the C counterpart:

	#define INIT_XML_ACTUAL_INIT
	#include "eschew.h"
	#include "the_header_you_created_above.h"

	void init_xml_config()
	{
		INIT_XML_TABLE(config);
		INIT_XML_TABLE(resolution);
		INIT_XML_TABLE(keys_player_1);
		INIT_XML_TABLE(keys_player_2);
		INIT_XML_TABLE(messages);
	}

	Call the function above at runtime, call read_xml() with the name of 
	your XML config file and make sure you include your 2 headers (without
	the ACTUAL_INIT define) in all the source you plan to use your tables,
	That's it!

