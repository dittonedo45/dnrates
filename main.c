/*XXX This Document was modified on 1636959908 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <jv.h>
#include <string.h>

xmlNodePtr dn_skip_txt ( xmlNodePtr d )
{
 while( d && d->name && !strcmp ( "text", d->name ) )
  d = d->next;
 return d;
}

xmlNodePtr dn_get_el ( char *str, xmlNodePtr d )
{

 if( !d )
  return 0;

 if( !str || !strlen ( str ) )
  return d;

 char *p = str;

 while( p && *p ) {
  char *t = strchr ( p, '|' );

  if( !t ) {
   d = dn_skip_txt ( d );
   if( !d || !d->name || strcmp ( d->name, p ) )
	return 0;
   break;
  } else if( t - p ) {
   char s[( t - p )];
   strncpy ( s, p, t - p );
   s[t - p] = 0;

   d = dn_skip_txt ( d );
   if( !d || !d->name || strncmp ( d->name, s, t - p ) ) {
	return 0;
   }

   d = d->children;

  }
  p = t + 1;
 }
 return d;
}

xmlNodePtr dn_get_ar ( char *str, xmlNodePtr d )
{

 if( !d )
  return 0;

 if( !str || !strlen ( str ) )
  return d;

 char *p = str;

 while( p && *p ) {
  char *t = strchr ( p, '|' );

  if( !t ) {
   d = dn_skip_txt ( d );

   while( d && d->name && strcmp ( d->name, p ) )
	d = d->next;

   return d;

   break;
  } else if( t - p ) {
   char s[( t - p )];
   strncpy ( s, p, t - p );
   s[t - p] = 0;

   d = dn_skip_txt ( d );

   while( d && d->name && strcmp ( s, p ) ) ;

   d = d->next;

  }
  p = t + 1;
 }

 return 0;
}

char *dn_el_get_str ( xmlNodePtr ti )
{
 if( !ti )
  return 0;
 if( !ti->children )
  return 0;
 return ( ti->children->content );
}

static jv dn_array ( char *str )
{
 char sep[] = ", ";

 char *p = str;

 jv a = jv_array (  );

 while( p ) {
  char *t = strstr ( p, sep );

  if( !t ) {
   a = jv_array_append ( a, jv_string ( p ) );
   break;
  } else {
   a = jv_array_append ( a, jv_string_fmt ( "%.*s", t - p, p ) );
  }

  p = ( t + strlen ( sep ) );
 }

 return jv_copy ( a );
}

int main ( signed Argsc, char *( Args[] ) )
{
 xmlDocPtr doc = xmlParseFile ( Args[1] );

 if( doc ) {
  xmlNodePtr d = dn_get_el ( "QALCULATE|category", doc->children );

  jv mq = jv_array (  );

  if( d && d->name ) {
   d = d->children;
   while( d ) {
	if( d->name && !strcmp ( d->name, "builtin_unit" ) ) {

	 char *unit = 0;
	 {
	  xmlNodePtr p = ( d->properties->children );
	  while( p ) {
	   unit = ( p->content );
	   p = p->next;
	  }
	 }

	 xmlNodePtr p = dn_skip_txt ( d->children );
	 char *ti = dn_el_get_str ( dn_get_ar ( "title", p ) );
	 char *countries = dn_el_get_str ( dn_get_ar ( "countries", p ) );

	 if( ti && countries ) {
	  jv obj = jv_object (  );
	  obj = jv_object_set ( obj, jv_string ( "unit" ), jv_string ( unit ) );
	  obj = jv_object_set ( obj, jv_string ( "name" ), jv_string ( ti ) );
	  obj =
	      jv_object_set ( obj, jv_string ( "countries" ),
	                      dn_array ( countries ) );
	  mq = jv_array_append ( mq, obj );
	 }
	}
	d = d->next;
   }

   jv_dumpf ( mq, stdout, 0 );
  }

  xmlFreeDoc ( doc );
 }

 return 0;
}
