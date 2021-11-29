/*XXX This Document was modified on 1637133315 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml2/libxml/parser.h>
#include <jv.h>
#include <string.h>

xmlNodePtr dn_skip_txt(xmlNodePtr d)
{
    while (d && d->name && !strcmp("text", d->name))
	d = d->next;
    return d;
}

xmlNodePtr dn_get_el(char *str, xmlNodePtr d)
{

    if (!d)
	return 0;

    if (!str || !strlen(str))
	return d;

    char *p = str;

    while (p && *p) {
	char *t = strchr(p, '|');

	if (!t) {
	    d = dn_skip_txt(d);
	    if (!d || !d->name || strcmp(d->name, p))
		return 0;
	    break;
	} else if (t - p) {
	    char s[(t - p)];
	    strncpy(s, p, t - p);
	    s[t - p] = 0;

	    d = dn_skip_txt(d);
	    if (!d || !d->name || strncmp(d->name, s, t - p)) {
		return 0;
	    }

	    d = d->children;

	}
	p = t + 1;
    }
    return d;
}

xmlNodePtr dn_get_ar(char *str, xmlNodePtr d)
{

    if (!d)
	return 0;

    if (!str || !strlen(str))
	return d;

    char *p = str;

    while (p && *p) {
	char *t = strchr(p, '|');

	if (!t) {
	    d = dn_skip_txt(d);

	    while (d && d->name && strcmp(d->name, p))
		d = d->next;

	    return d;

	    break;
	} else if (t - p) {
	    char s[(t - p)];
	    strncpy(s, p, t - p);
	    s[t - p] = 0;

	    d = dn_skip_txt(d);

	    while (d && d->name && strcmp(s, p));

	    d = d->next;

	}
	p = t + 1;
    }

    return 0;
}

char *dn_el_get_str(xmlNodePtr ti)
{
    if (!ti)
	return 0;
    if (!ti->children)
	return 0;
    return (ti->children->content);
}

static jv dn_array(char *str)
{
    char sep[] = ", ";

    char *p = str;

    jv a = jv_array();

    while (p) {
	char *t = strstr(p, sep);

	if (!t) {
	    a = jv_array_append(a, jv_string(p));
	    break;
	} else {
	    a = jv_array_append(a, jv_string_fmt("%.*s", t - p, p));
	}

	p = (t + strlen(sep));
    }

    return jv_copy(a);
}

#include <sys/stat.h>

static jv rt_load_json(char *path)
{
    struct stat st;
    if (!path)
	return jv_invalid();

    if (stat(path, &st) || !S_ISREG(st.st_mode))
	return jv_invalid();

    xmlDocPtr doc = xmlParseFile(path);

    if (!doc)
	return jv_invalid();

    xmlNodePtr d = dn_get_el("QALCULATE|category", doc->children);

    jv mq = jv_array();

    if (!d || !d->name)
	goto end;

    d = d->children;

    while (d) {
	if (d->name && !strcmp(d->name, "builtin_unit")) {

	    char *unit = 0;
	    {
		xmlNodePtr p = (d->properties->children);
		while (p) {
		    unit = (p->content);
		    p = p->next;
		}
	    }

	    xmlNodePtr p = dn_skip_txt(d->children);
	    char *ti = dn_el_get_str(dn_get_ar("title", p));
	    char *countries = dn_el_get_str(dn_get_ar("countries", p));

	    if (ti && countries) {
		jv obj = jv_object();
		obj =
		    jv_object_set(obj, jv_string("unit"), jv_string(unit));
		obj = jv_object_set(obj, jv_string("name"), jv_string(ti));
		obj =
		    jv_object_set(obj, jv_string("countries"),
				  dn_array(countries));
		mq = jv_array_append(mq, obj);
	    }
	}
	d = d->next;
    }
  end:
    xmlFreeDoc(doc);
    return jv_copy(mq);

}

#if 1				// Folks, This Recursive Function is for printing the nodes, in a tree like form (Just To Grasp The Flow)
static void print_n(xmlNodePtr n, int i)
{
    if (!n)
	return;

    if (n->name) {
	int a = i;
	while (a > 0)
	    putchar(' '), a--;
	printf("\033[32;1m+\033[0m%s {", n->name);
	if (n->properties) {
	    xmlAttrPtr ap = n->properties;
	    while (ap) {
		xmlNodePtr ans = ap->children;
		printf("%s=%s", ap->name, ans->content);
		ap = ap->next;
		if (ap)
		    printf(",");
	    }
	}
	printf("}\n");
    }
    print_n(n->children, i + 1);
    if (n->name) {
	int a = i;
	while (a > 0)
	    putchar(' '), a--;
	printf("\033[33;1m-\033[0m%s\n", n->name);
    }
    if (n->next) {
	// putchar ( '\n' );
	print_n(n->next, i);
    }
}
#endif

static jv rt_deal_with_siblings(xmlNodePtr n)
{
    jv rates = jv_array();

    if (!n) {
	return jv_copy(rates);
    }

    while (n) {
	xmlAttrPtr nr = n->properties;
	jv mo = jv_object();

	while (nr) {
	    xmlNodePtr txt = nr->children;
	    mo = jv_object_set(mo, jv_string(nr->name),
			       jv_string(txt->content));
	    nr = nr->next;
	}

	rates = jv_array_append(rates, mo);
	n = n->next;
    }

    return jv_copy(rates);
}

jv rt_get_file(char *path);
jv rt_get_url(char *path);
jv rt_get_unit(jv, char *);
void rt_show(jv, jv, jv);

int main(signed Argsc, char *(Args[]))
{
    jv currencies = rt_load_json("currencies.xml");
#if 1
    jv rates =
	rt_get_url
	("https://www.ecb.europa.eu/stats/eurofxref/eurofxref-hist-90d.xml");
#else
    jv rates = rt_get_url("./rates.xml");
#endif
    if (!jv_is_valid(currencies))
	return 1;
    if (!jv_is_valid(rates))
	return 2;

    jv stack = jv_object_get(rates, jv_string("stack"));

// Foreach, Element
    for (int l = jv_array_length(jv_copy(stack)) - 1; l > -1; l--) {
	jv el = jv_array_get(jv_copy(stack), l);

	jv rates = jv_object_get(jv_copy(el), jv_string("rates"));
	jv date = jv_object_get(jv_copy(el), jv_string("time"));

	rt_show(date, rates, currencies);
    }

    return 0;
}

void rt_show(jv date, jv rates, jv currencies)
{

    jv_array_foreach(jv_copy(rates), i, el)
	//
    {
	jv curr = jv_object_get(jv_copy(el), jv_string("currency"));
	if (!strcmp(jv_string_value(curr), "USD")) {
	    jv_dumpf(date, stdout, 0);
	    jv_dumpf(el, stdout, 0);
	}
    }
}

jv rt_get_unit(jv ar, char *u)
{
    jv_array_foreach(jv_copy(ar), i, el) {
	jv unit = jv_object_get(jv_copy(el), jv_string("unit"));
	if (jv_is_valid(unit)) {
	    if (!strcmp(jv_string_value(unit), u))
		return jv_copy(el);
	}
    }
    return jv_invalid();
}

jv rt_get_file(char *path)
{
    jv maina = jv_object();
    jv arr = jv_array();

    xmlDocPtr doc = xmlParseFile(path);

    if (!doc)
	return jv_invalid();

    xmlNodePtr nd = dn_get_el("Envelope", doc->children);
    xmlNodePtr subject = dn_get_el("Envelope|subject", nd);
    xmlNodePtr cubes = dn_get_ar("Cube", nd->children);

    xmlNodePtr sender = dn_get_ar("Sender", nd->children);
    do {
	if (!sender)
	    break;
	sender = dn_get_el("Sender|name", sender);
	if (!sender)
	    break;
	sender = (sender->children);
	if (!sender)
	    break;
	maina = jv_object_set(maina, jv_string("sender"), jv_string
			      (sender->content));
    } while (0);

    do {
	if (!cubes || !cubes->children)
	    break;

	int i = 1;

	jv obj = jv_object();

	xmlNodePtr p = cubes->children;
	while (p) {
	    {
		xmlAttrPtr n = p->properties;

		if (!n || !n->name) {
		    p = p->next;
		    continue;
		}

		obj = jv_object();
		obj = jv_object_set(obj, jv_string(n->name),
				    jv_string(n->children->content));

		obj =
		    jv_object_set(obj, jv_string("rates"),
				  rt_deal_with_siblings(p->children));
		arr = jv_array_append(arr, obj);
		p = p->next;
	    }
	}

	break;
    }
    while (0);

    maina = jv_object_set(maina, jv_string("stack"), jv_copy(arr));

    return jv_copy(maina);
}

#if 1
#include <libavformat/avio.h>

static int rt_avio_read(void *d, char *b, int l)
{
    int ret = 0;

    do {
	ret = avio_read((AVIOContext *) d, b, l);
	/// In case, the download get some petty errors ?
    } while (ret != AVERROR_EOF && ret < 0);

    return ret;
}

static int rt_avio_close(void *d)
{
    avio_close((AVIOContext *) d);
    return 0;
}

jv rt_get_url(char *path)
{
    jv maina = jv_object();
    jv arr = jv_array();
    AVIOContext *io = NULL;

    int ret = 0;

    av_log_set_callback(0);
    ret = avio_open(&io, path, AVIO_FLAG_READ);

    if (ret < 0)
	return jv_invalid();

    xmlDocPtr doc =
	xmlReadIO(rt_avio_read, rt_avio_close, (void *) io, path, "UTF-8",
		  0);

    if (!doc)
	return jv_invalid();

    xmlNodePtr nd = dn_get_el("Envelope", doc->children);
    xmlNodePtr subject = dn_get_el("Envelope|subject", nd);
    xmlNodePtr cubes = dn_get_ar("Cube", nd->children);

    xmlNodePtr sender = dn_get_ar("Sender", nd->children);
    do {
	if (!sender)
	    break;
	sender = dn_get_el("Sender|name", sender);
	if (!sender)
	    break;
	sender = (sender->children);
	if (!sender)
	    break;
	maina = jv_object_set(maina, jv_string("sender"), jv_string
			      (sender->content));
    } while (0);

    do {
	if (!cubes || !cubes->children)
	    break;

	int i = 1;

	jv obj = jv_object();

	xmlNodePtr p = cubes->children;
	while (p) {
	    {
		xmlAttrPtr n = p->properties;

		if (!n || !n->name) {
		    p = p->next;
		    continue;
		}

		obj = jv_object();
		obj = jv_object_set(obj, jv_string(n->name),
				    jv_string(n->children->content));

		obj =
		    jv_object_set(obj, jv_string("rates"),
				  rt_deal_with_siblings(p->children));
		arr = jv_array_append(arr, obj);
		p = p->next;
	    }
	}

	break;
    }
    while (0);

    maina = jv_object_set(maina, jv_string("stack"), jv_copy(arr));

    return jv_copy(maina);
}
#endif
