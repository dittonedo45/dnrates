/*XXX This Document was modified on 1637057491 */
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

#include <sys/stat.h>

static jv rt_load_json ( char *path )
{
 struct stat st;
 if( !path )
  return jv_invalid (  );

 if( stat ( path, &st ) || !S_ISREG ( st.st_mode ) )
  return jv_invalid (  );

 xmlDocPtr doc = xmlParseFile ( path );

 if( !doc )
  return jv_invalid (  );

 xmlNodePtr d = dn_get_el ( "QALCULATE|category", doc->children );

 jv mq = jv_array (  );

 if( !d || !d->name )
  goto end;

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
 end:
 xmlFreeDoc ( doc );
 return jv_copy ( mq );

}

#if 1                           // Folks, This Recursive Function is for printing the nodes, in a tree like form (Just To Grasp The Flow)
static void print_n ( xmlNodePtr n, int i )
{
 if( !n )
  return;

 if( n->name ) {
  int a = i;
  while( a > 0 )
   putchar ( ' ' ), a--;
  printf ( "\033[32;1m+\033[0m%s {", n->name );
  if( n->properties ) {
   xmlAttrPtr ap = n->properties;
   while( ap ) {
	xmlNodePtr ans = ap->children;
	printf ( "%s=%s", ap->name, ans->content );
	ap = ap->next;
	if( ap )
	 printf ( "," );
   }
  }
  printf ( "}\n" );
 }
 print_n ( n->children, i + 1 );
 if( n->name ) {
  int a = i;
  while( a > 0 )
   putchar ( ' ' ), a--;
  printf ( "\033[33;1m-\033[0m%s\n", n->name );
 }
 if( n->next ) {
  // putchar ( '\n' );
  print_n ( n->next, i );
 }
}
#endif

static jv rt_deal_with_siblings ( xmlNodePtr n )
{
 jv rates = jv_array (  );

 if( !n ) {
  return jv_copy ( rates );
 }

 while( n ) {
  xmlAttrPtr nr = n->properties;
  jv mo = jv_object (  );

  while( nr ) {
   xmlNodePtr txt = nr->children;
   mo = jv_object_set ( mo, jv_string ( nr->name ),
                        jv_string ( txt->content ) );
   nr = nr->next;
  }

  rates = jv_array_append ( rates, mo );
  n = n->next;
 }

 return jv_copy ( rates );
}

jv rt_get_file ( char *path );

int main ( signed Argsc, char *( Args[] ) )
{
 jv currencies = rt_load_json ( "currencies.xml" );
 jv rates = rt_get_file ( "./rates.xml" );

}

jv rt_get_file ( char *path )
{
 jv maina = jv_object (  );
 jv arr = jv_array (  );

 xmlDocPtr doc = xmlParseFile ( path );

 if( !doc )
  return jv_invalid (  );

 xmlNodePtr nd = dn_get_el ( "Envelope", doc->children );
 xmlNodePtr subject = dn_get_el ( "Envelope|subject", nd );
 xmlNodePtr cubes = dn_get_ar ( "Cube", nd->children );

 xmlNodePtr sender = dn_get_ar ( "Sender", nd->children );
 do {
  if( !sender )
   break;
  sender = dn_get_el ( "Sender|name", sender );
  if( !sender )
   break;
  sender = ( sender->children );
  if( !sender )
   break;
  maina = jv_object_set ( maina, jv_string ( "sender" ), jv_string
                          ( sender->content ) );
 } while( 0 );

 do {
  if( !cubes || !cubes->children )
   break;

  int i = 1;

  jv obj = jv_object (  );

  xmlNodePtr p = cubes->children;
  while( p ) {
   {
	xmlAttrPtr n = p->properties;

	if( !n || !n->name ) {
	 p = p->next;
	 continue;
	}

	obj = jv_object (  );
	obj = jv_object_set ( obj, jv_string ( n->name ),
	                      jv_string ( n->children->content ) );

	obj =
	    jv_object_set ( obj, jv_string ( "rates" ),
	                    rt_deal_with_siblings ( p->children ) );
	arr = jv_array_append ( arr, obj );
	p = p->next;
   }
  }

  break;
 }
 while( 0 );

 maina = jv_object_set ( maina, jv_string ( "stack" ), jv_copy ( arr ) );

 return jv_copy ( maina );
}
