                     
               ESCHEW - Even Simpler C-Heuristics Expat Wrapper
                  
                             USAGE NOTES


* OVERVIEW:
Eschew is a simple tables constructor + runtime XML parser wrapper based on
Expat. The typical usage of Eschew is to read/write application options from an
XML configuration.
 
The primary goal of Eschew, and what differentiates it from other XML wrappers,
is its ability to instantiate arrays for holding XML values that are readily
available at compilation time, just like any other standard C array.
This unique feature makes the use of Eschew both extremely efficient and very
straightforward. 

* ESCHEW vs SCEW, or why not use SCEW (Simple C Expat Wrapper) instead?
1. Scew's node values are string only, whereas Eschew can define and populate 
   any type (from booleans to floating point, including any type of signed or 
   unsigned integers)
2. Scew's nodes are only available at runtime which results in massive overhead
   everytime accessing a value is required. If for instance you define user 
   input controls in your configuration XML, this is not a viable option. 
   Eschew on the other hand, provides typed table values that are *directly* 
   accessible at compilation time, whilst still being populated from the XML at
   runtime
3. Scew provides a number of features that aren't really useful for simple xml
   configuration I/O. Eschew does away with these features to keep as efficient
   as possible

Once again, the main advantage of Eschew is that, after the XML file has been 
read, NO time consuming resolution need to occur to access an XML value. They 
are available like any other regular C array of values (with the node names 
having automatically defined as enums by Eschew, to facilitate access). 

For instance, if you use the Eschew macros to create the keys table for player_1 
(which includes the node "dir_up"), you will be able to use directly in your 
source:

  if (current_key = xml_keys_player_1.value[dir_up])
  {
    // Perform direction up action
    ...
  }

* FEATURES
- Read and write XML configuration files (Re-write of XML conf possible)
- Preservation/Creation of comments
- Support of (single) attributes

* LIMITATIONS
  The following limitations are in effect when using Eschew:
- Only one attribute can be declared per node, which is used to differentiate 
  between nodes of the same name. For more information, see the example below 
  for the "keys" node
- Child nodes of a specific branch must be of the same type (because they all 
  belong to the same C array). A small exception for this is for boolean values
  that will be automatically interpreted for any integer table that is being 
  read. This means that value like "true", "enabled", "on", "false", "disable", 
  "off" will be detected and set accordingly (i.e. false -> 0, true -> non zero)
- The names you provide for XML nodes must not conflict with C keywords or other
  variables defined in your source. This is because node names are automatically
  added as enums
- Node definitions must follow the logical order, i.e. parent nodes must be 
  instantiated before of their children.
- You cannot use underscores in your base table names, as underscores are used
  to separate attributes properties (Note that this can be changed by setting 
  the XML_ATTRIBUTE_SEPARATOR to something other than "_" in eschew.h)
- Maximum depth for XML nodes is 16 (again, can be changed in eschew.h)
- You must use one of the xml_#### types as a type when defining your arrays 
  (eg: "xml_unsigned_long" instead of "unsigned long"). These are just redefini-
  tions of the stantard C types, required as the array instantiation macros can
  only work with a single word type
 
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

With Eschew, you just need to create a header with the following:

DEFINE_XML_NODES(config_nodes, resolution, keys, messages)
CREATE_XML_TABLE(config, config_nodes, xml_node) 
SET_XML_ROOT(config)
DEFINE_XML_NODES(res_nodes, scr_width, scr_height, fullscreen)
CREATE_XML_TABLE(resolution, res_nodes, xml_int)
DEFINE_XML_NODES(keys_nodes, dir_left, dir_right, dir_up, dir_down)
CREATE_XML_TABLE(keys_player_1, keys_nodes, xml_unsigned_char)
CREATE_XML_TABLE(keys_player_2, keys_nodes, xml_unsigned_char)
DEFINE_XML_NODES(msg_nodes, msg1, msg2)
CREATE_XML_TABLE(messages, msg_nodes, xml_string)

This results in the creation of the following tables which you can
use right away (with the content initialized at runtime)

int		xml_resolution.value[3]
unsigned char	xml_keys_player_1.value[4]
unsigned char	xml_keys_player_2.value[4]
char*		xml_messages.values[2]

The following enums are also automatically set:

enum { resolution, keys_player_1, keys_player_2, messages };
enum { scr_width, scr_height, fullscreen };
enum { dir_left, dir_right, dir_up, dir_down };
enum { msg1, msg2 };

If needed, the names and comments for the nodes are also available in 
xml_<table_name>.name[i] and xml_<table_name>.comment[i] respectively

Then at runtime, you would call INIT_XML_TABLE() for each one of the tables
above, read_xml() to populate them from the relevant XML file and write_xml(),
if needed, to save the configuration values back.

For an example of use, see the example.c included in the source