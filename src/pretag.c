/*
    pmacct (Promiscuous mode IP Accounting package)
    pmacct is Copyright (C) 2003-2013 by Paolo Lucente
*/

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/* defines */
#define __PRETAG_C

/* includes */
#include "pmacct.h"
#include "nfacctd.h"
#include "pretag_handlers.h"
#include "pretag-data.h"
#include "tee_plugin/tee_recvs.h"
#include "tee_plugin/tee_recvs-data.h"
#include "isis/isis.h"
#include "isis/isis-data.h"
#include "crc32.c"

/*
   XXX: load_id_file() interface cleanup pending:
   - if a table is tag-related then it is passed as argument t
   - else it is passed as argument req->key_value_table 
*/
void load_id_file(int acct_type, char *filename, struct id_table *t, struct plugin_requests *req, int *map_allocated)
{
  struct id_table tmp;
  struct id_entry *ptr, *ptr2;
  FILE *file;
  char buf[LARGEBUFLEN];
  int v4_num = 0, x, tot_lines = 0, err, index, label_solved, sz;
  int ignoring, read_len;
  struct stat st;

#if defined ENABLE_IPV6
  int v6_num = 0;
#endif

  if (!map_allocated) return;

  /* parsing engine vars */
  char *start, *key = NULL, *value = NULL;
  int len;

  Log(LOG_INFO, "INFO ( %s/%s ): Trying to (re)load map: %s\n", config.name, config.type, filename);

  memset(&st, 0, sizeof(st));
  memset(&tmp, 0, sizeof(struct id_table));

  if (!config.pre_tag_map_entries) config.pre_tag_map_entries = MAX_PRETAG_MAP_ENTRIES;

  if (filename) {
    if ((file = fopen(filename, "r")) == NULL) {
      Log(LOG_ERR, "ERROR ( %s/%s ): map '%s' not found.\n", config.name, config.type, filename);
      goto handle_error;
    }

    sz = sizeof(struct id_entry)*config.pre_tag_map_entries;

    if (t) {
      if (*map_allocated == 0) {
        memset(t, 0, sizeof(struct id_table));
        t->e = (struct id_entry *) malloc(sz);
	if (!t->e) {
	  Log(LOG_ERR, "ERROR ( %s/%s ): malloc() failed (load_id_file)\n", config.name, config.type);
	  goto handle_error;
	}
        *map_allocated = TRUE;
      }
      else {
        ptr = t->e ;

        if (config.pre_tag_map_index) {
	  if (acct_type == ACCT_NF || acct_type == ACCT_SF || acct_type == ACCT_PM)
	    pretag_index_destroy(t, filename);
	}

        memset(t, 0, sizeof(struct id_table));
        t->e = ptr ;
      }
    }
    else {
      *map_allocated = TRUE;
    }

    tmp.e = (struct id_entry *) malloc(sz);
    if (!tmp.e) {
      Log(LOG_ERR, "ERROR ( %s/%s ): malloc() failed (load_id_file)\n", config.name, config.type);
      goto handle_error;
    }

    memset(tmp.e, 0, sz);
    if (t) memset(t->e, 0, sz);

    if (acct_type == MAP_IGP) {
      igp_daemon_map_initialize(filename, req);
      read_len = LARGEBUFLEN;
    }
    else read_len = SRVBUFLEN;

    /* first stage: reading Agent ID file and arranging it in a temporary memory table */
    while (!feof(file)) {
      ignoring = FALSE;
      req->line_num = ++tot_lines;

      if (tmp.num >= config.pre_tag_map_entries) {
	Log(LOG_WARNING, "WARN ( %s/%s ): map '%s' cut to the first %u entries. Number of entries can be configured via 'pre_tag_map_etries'.\n",
		config.name, config.type, filename, config.pre_tag_map_entries);
	break;
      }
      memset(buf, 0, read_len);
      if (fgets(buf, read_len, file)) {
        if (!iscomment(buf) && !isblankline(buf)) {
	  if (strlen(buf) == (read_len-1) && !strchr(buf, '\n')) {
	    Log(LOG_WARNING, "WARN ( %s/%s ): line too long (max %u chars). Line %d in map '%s' ignored.\n",
			config.name, config.type, read_len, tot_lines, filename);
	    continue;
	  }
          if (!check_not_valid_char(filename, buf, '|')) {
            mark_columns(buf);
            trim_all_spaces(buf);
	    strip_quotes(buf);

	    /* resetting the entry and enforcing defaults */
	    if (acct_type == MAP_IGP) memset(&ime, 0, sizeof(ime));
            memset(&tmp.e[tmp.num], 0, sizeof(struct id_entry));
	    tmp.e[tmp.num].ret = FALSE;

            err = FALSE; key = NULL; value = NULL;
            start = buf;
            len = strlen(buf);

            for (x = 0; x <= len; x++) {
              if (buf[x] == '=') {
                if (start == &buf[x]) continue;
                if (!key) {
                  buf[x] = '\0';
		  key = start;
                  x++;
                  start = &buf[x];
		}
              }
              if ((buf[x] == '|') || (buf[x] == '\0')) {
                if (start == &buf[x]) continue;
                buf[x] = '\0';
                if (value || !key) {
                  Log(LOG_ERR, "ERROR ( %s/%s ): malformed line %d in map '%s'. Ignored.\n", config.name, config.type, tot_lines, filename);
                  err = TRUE;
                  break;
                }
                else value = start;
                x++;
                start = &buf[x];
              }

              if (key && value) {
                int dindex; /* dictionary index */

		/* Processing of source BGP-related primitives kept consistent;
		   This can indeed be split as required in future */
		if (acct_type == MAP_BGP_PEER_AS_SRC || acct_type == MAP_BGP_SRC_LOCAL_PREF ||
		    acct_type == MAP_BGP_SRC_MED) {
                  for (dindex = 0; strcmp(bpas_map_dictionary[dindex].key, ""); dindex++) {
                    if (!strcmp(bpas_map_dictionary[dindex].key, key)) {
                      err = (*bpas_map_dictionary[dindex].func)(filename, &tmp.e[tmp.num], value, req, acct_type);
                      break;
                    }
                    else err = E_NOTFOUND; /* key not found */
                  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n",
						config.name, config.type, key, tot_lines, filename);
                    else Log(LOG_ERR, "Line %d ignored.\n", tot_lines);
                    break;
                  }
                  key = NULL; value = NULL;
		}
		else if (acct_type == MAP_BGP_TO_XFLOW_AGENT) {
                  for (dindex = 0; strcmp(bta_map_dictionary[dindex].key, ""); dindex++) {
                    if (!strcmp(bta_map_dictionary[dindex].key, key)) {
                      err = (*bta_map_dictionary[dindex].func)(filename, &tmp.e[tmp.num], value, req, acct_type);
                      break;
                    }
                    else err = E_NOTFOUND; /* key not found */
                  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n", 
						config.name, config.type, key, tot_lines, filename);
                    else Log(LOG_ERR, "Line %d ignored.\n", tot_lines);
                    break;
                  }
                  key = NULL; value = NULL;
		}
                else if (acct_type == MAP_FLOW_TO_RD) {
                  for (dindex = 0; strcmp(bitr_map_dictionary[dindex].key, ""); dindex++) {
                    if (!strcmp(bitr_map_dictionary[dindex].key, key)) {
                      err = (*bitr_map_dictionary[dindex].func)(filename, &tmp.e[tmp.num], value, req, acct_type);
                      break;
                    }
                    else err = E_NOTFOUND; /* key not found */
                  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n", 
						config.name, config.type, key, tot_lines, filename);
                    else Log(LOG_ERR, "Line %d ignored.\n", tot_lines);
                    break;
                  }
                  key = NULL; value = NULL;
                }
		else if (acct_type == MAP_SAMPLING) {
                  for (dindex = 0; strcmp(sampling_map_dictionary[dindex].key, ""); dindex++) {
                    if (!strcmp(sampling_map_dictionary[dindex].key, key)) {
                      err = (*sampling_map_dictionary[dindex].func)(filename, &tmp.e[tmp.num], value, req, acct_type);
                      break;
                    }
                    else err = E_NOTFOUND; /* key not found */
                  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n", 
						config.name, config.type, key, tot_lines, filename);
                    else Log(LOG_ERR, "Line %d ignored.\n", tot_lines);
                    break;
                  }
                  key = NULL; value = NULL;
		}
		else if (acct_type == MAP_TEE_RECVS) {
                  for (dindex = 0; strcmp(tee_recvs_map_dictionary[dindex].key, ""); dindex++) {
                    if (!strcmp(tee_recvs_map_dictionary[dindex].key, key)) {
                      err = (*tee_recvs_map_dictionary[dindex].func)(filename, NULL, value, req, acct_type);
                      break;
                    }
                    else err = E_NOTFOUND; /* key not found */
                  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n", 
						config.name, config.type, key, tot_lines, filename);
                    else Log(LOG_ERR, "Line %d ignored in map '%s'.\n", tot_lines, filename);
                    break;
                  }
                  key = NULL; value = NULL;
		}
                else if (acct_type == MAP_IGP) {
                  for (dindex = 0; strcmp(igp_daemon_map_dictionary[dindex].key, ""); dindex++) {
                    if (!strcmp(igp_daemon_map_dictionary[dindex].key, key)) {
                      err = (*igp_daemon_map_dictionary[dindex].func)(filename, NULL, value, req, acct_type);
                      break;
                    }
                    else err = E_NOTFOUND; /* key not found */
                  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n",
                                                config.name, config.type, key, tot_lines, filename);
                    else {
		      Log(LOG_ERR, "Line %d ignored in map '%s'.\n", tot_lines, filename);
		      ignoring = TRUE;
		    }
                  }
                  key = NULL; value = NULL;
                }
                else if (acct_type == MAP_CUSTOM_PRIMITIVES) {
                  for (dindex = 0; strcmp(custom_primitives_map_dictionary[dindex].key, ""); dindex++) {
                    if (!strcmp(custom_primitives_map_dictionary[dindex].key, key)) {
                      err = (*custom_primitives_map_dictionary[dindex].func)(filename, &tmp.e[tmp.num], value, req, acct_type);
                      break;
                    }
                    else err = E_NOTFOUND; /* key not found */
                  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n",
                                                config.name, config.type, key, tot_lines, filename);
                    else Log(LOG_ERR, "Line %d ignored.\n", tot_lines);
                    break;
                  }
                  key = NULL; value = NULL;
		}
		else {
		  if (tee_plugins) {
                    for (dindex = 0; strcmp(tag_map_tee_dictionary[dindex].key, ""); dindex++) {
                      if (!strcmp(tag_map_tee_dictionary[dindex].key, key)) {
                        err = (*tag_map_tee_dictionary[dindex].func)(filename, &tmp.e[tmp.num], value, req, acct_type);
                        break;
                      }
                      else err = E_NOTFOUND; /* key not found */
                    }
		  }
		  else {
                    for (dindex = 0; strcmp(tag_map_dictionary[dindex].key, ""); dindex++) {
                      if (!strcmp(tag_map_dictionary[dindex].key, key)) {
                        err = (*tag_map_dictionary[dindex].func)(filename, &tmp.e[tmp.num], value, req, acct_type);
                        break;
                      }
                      else err = E_NOTFOUND; /* key not found */
		    }
		  }
                  if (err) {
                    if (err == E_NOTFOUND) Log(LOG_ERR, "ERROR ( %s/%s ): unknown key '%s' at line %d in map '%s'. Ignored.\n", 
						config.name, config.type, key, tot_lines, filename);
                    else Log(LOG_ERR, "Line %d ignored in map '%s'.\n", tot_lines, filename);
                    break; 
                  }
                  key = NULL; value = NULL;
		}
              }
            }
	    if (!ignoring) {
              /* verifying errors and required fields */
	      if (acct_type == ACCT_NF || acct_type == ACCT_SF) {
	        if (tmp.e[tmp.num].id && tmp.e[tmp.num].id2) 
		   Log(LOG_ERR, "ERROR ( %s/%s ): set_tag (id) and set_tag2 (id2) are mutual exclusive at line %d in map '%s'.\n", 
			config.name, config.type, tot_lines, filename);
                else if (!err && tmp.e[tmp.num].agent_ip.a.family) {
                  int j, z;

                  for (j = 0; tmp.e[tmp.num].func[j]; j++);
		  for (z = 0; tmp.e[tmp.num].set_func[z]; z++, j++) {
		    tmp.e[tmp.num].func[j] = tmp.e[tmp.num].set_func[z];
		    tmp.e[tmp.num].func_type[j] = tmp.e[tmp.num].set_func_type[z];
		  }

	          if (tmp.e[tmp.num].agent_ip.a.family == AF_INET) v4_num++;
#if defined ENABLE_IPV6
	          else if (tmp.e[tmp.num].agent_ip.a.family == AF_INET6) v6_num++;
#endif
                  tmp.num++;
                }
	        /* if any required field is missing and other errors have been signalled
	           before we will trap an error message */
	        else if (!err && !tmp.e[tmp.num].agent_ip.a.family)
	          Log(LOG_ERR, "ERROR ( %s/%s ): required key missing at line %d in map '%s'. Required key is: 'ip'.\n",
			config.name, config.type, tot_lines, filename); 
	      }
	      else if (acct_type == ACCT_PM) {
	        if (tmp.e[tmp.num].id && tmp.e[tmp.num].id2)
                   Log(LOG_ERR, "ERROR ( %s/%s ): set_tag (id) and set_tag2 (id2) are mutual exclusive at line %d in map '%s'.\n", 
			config.name, config.type, tot_lines, filename);
	        else if (tmp.e[tmp.num].agent_ip.a.family)
		  Log(LOG_ERR, "ERROR ( %s/%s ): key 'ip' not applicable here. Invalid line %d in map '%s'.\n",
			config.name, config.type, tot_lines, filename);
	        else if (!err) {
                  int j, z;

		  for (j = 0; tmp.e[tmp.num].func[j]; j++);
		  for (z = 0; tmp.e[tmp.num].set_func[z]; z++, j++) {
		    tmp.e[tmp.num].func[j] = tmp.e[tmp.num].set_func[z];
		    tmp.e[tmp.num].func_type[j] = tmp.e[tmp.num].set_func_type[z];
		  }
		  tmp.e[tmp.num].agent_ip.a.family = AF_INET; /* we emulate a dummy '0.0.0.0' IPv4 address */
		  v4_num++; tmp.num++;
	        }
	      }
	      else if (acct_type == MAP_BGP_PEER_AS_SRC || acct_type == MAP_BGP_SRC_LOCAL_PREF ||
	  	       acct_type == MAP_BGP_SRC_MED) {
                if (!err && (tmp.e[tmp.num].id || tmp.e[tmp.num].flags) && tmp.e[tmp.num].agent_ip.a.family) {
                  int j;

                  for (j = 0; tmp.e[tmp.num].func[j]; j++);
                  tmp.e[tmp.num].func[j] = pretag_id_handler;
                  if (tmp.e[tmp.num].agent_ip.a.family == AF_INET) v4_num++;
#if defined ENABLE_IPV6
                  else if (tmp.e[tmp.num].agent_ip.a.family == AF_INET6) v6_num++;
#endif
                  tmp.num++;
                }
                else if ((!tmp.e[tmp.num].id || !tmp.e[tmp.num].agent_ip.a.family) && !err)
                  Log(LOG_ERR, "ERROR ( %s/%s ): required key missing at line %d in map '%s'. Required keys are: 'id', 'ip'.\n", 
			config.name, config.type, tot_lines, filename);
	      }
              else if (acct_type == MAP_BGP_TO_XFLOW_AGENT) {
                if (!err && tmp.e[tmp.num].id && tmp.e[tmp.num].agent_ip.a.family) {
                  int j, z;

                  for (j = 0; tmp.e[tmp.num].func[j]; j++);
		  for (z = 0; tmp.e[tmp.num].set_func[z]; z++, j++) {
                    tmp.e[tmp.num].func[j] = tmp.e[tmp.num].set_func[z];
                    tmp.e[tmp.num].func_type[j] = tmp.e[tmp.num].set_func_type[z];
                  }
		  /* imposing pretag_id_handler to be the last one */
		  tmp.e[tmp.num].func[j] = pretag_id_handler;

                  if (tmp.e[tmp.num].agent_ip.a.family == AF_INET) v4_num++;
#if defined ENABLE_IPV6
                  else if (tmp.e[tmp.num].agent_ip.a.family == AF_INET6) v6_num++;
#endif
                  tmp.num++;
                }
                else if ((!tmp.e[tmp.num].id || !tmp.e[tmp.num].agent_ip.a.family) && !err)
                  Log(LOG_ERR, "ERROR ( %s/%s ): required key missing at line %d in map '%s'. Required keys are: 'id', 'ip'.\n",
                        config.name, config.type, tot_lines, filename);
              }
              else if (acct_type == MAP_FLOW_TO_RD) {
                if (!err && tmp.e[tmp.num].id && tmp.e[tmp.num].agent_ip.a.family) {
                  int j;

                  for (j = 0; tmp.e[tmp.num].func[j]; j++);
                  tmp.e[tmp.num].func[j] = pretag_id_handler;
                  if (tmp.e[tmp.num].agent_ip.a.family == AF_INET) v4_num++;
#if defined ENABLE_IPV6
                  else if (tmp.e[tmp.num].agent_ip.a.family == AF_INET6) v6_num++;
#endif
                  tmp.num++;
                }
	      }
              else if (acct_type == MAP_SAMPLING) {
                if (!err && tmp.e[tmp.num].id && tmp.e[tmp.num].agent_ip.a.family) {
                  int j;

                  for (j = 0; tmp.e[tmp.num].func[j]; j++);
                  tmp.e[tmp.num].func[j] = pretag_id_handler;
                  if (tmp.e[tmp.num].agent_ip.a.family == AF_INET) v4_num++;
#if defined ENABLE_IPV6
                  else if (tmp.e[tmp.num].agent_ip.a.family == AF_INET6) v6_num++;
#endif
                  tmp.num++;
                }
                else if ((!tmp.e[tmp.num].id || !tmp.e[tmp.num].agent_ip.a.family) && !err)
                  Log(LOG_ERR, "ERROR ( %s/%s ): required key missing at line %d in map '%s'. Required keys are: 'id', 'ip'.\n",
			config.name, config.type, tot_lines, filename);
              }
	      else if (acct_type == MAP_TEE_RECVS) tee_recvs_map_validate(filename, req); 
	      else if (acct_type == MAP_IGP) igp_daemon_map_validate(filename, req); 
	      else if (acct_type == MAP_CUSTOM_PRIMITIVES) custom_primitives_map_validate(filename, req); 
	    }
          }
          else Log(LOG_ERR, "ERROR ( %s/%s ): malformed line %d in map '%s'. Ignored.\n",
			config.name, config.type, tot_lines, filename);
        }
      }
    }
    fclose(file);

    if (acct_type == MAP_IGP) igp_daemon_map_finalize(filename, req);

    if (t) {
      stat(filename, &st);
      t->timestamp = st.st_mtime;

      /* second stage: segregating IPv4, IPv6. No further reordering
         in order to deal smoothly with jumps (ie. JEQs) */

      x = 0;
      t->type = acct_type;
      t->num = tmp.num;
      t->ipv4_num = v4_num; 
      t->ipv4_base = &t->e[x];
      for (index = 0; index < tmp.num; index++) {
        if (tmp.e[index].agent_ip.a.family == AF_INET) { 
          memcpy(&t->e[x], &tmp.e[index], sizeof(struct id_entry));
	  t->e[x].pos = x;
	  x++;
        }
      }
#if defined ENABLE_IPV6
      t->ipv6_num = v6_num;
      t->ipv6_base = &t->e[x];
      for (index = 0; index < tmp.num; index++) {
        if (tmp.e[index].agent_ip.a.family == AF_INET6) {
          memcpy(&t->e[x], &tmp.e[index], sizeof(struct id_entry));
	  t->e[x].pos = x;
          x++;
        }
      }
#endif

      /* third stage: building short circuits basing on jumps and labels. Only
         forward references are solved. Backward and unsolved references will
         generate errors. */
      for (ptr = t->ipv4_base, x = 0; x < t->ipv4_num; ptr++, x++) {
        if (ptr->jeq.label) {
	  label_solved = FALSE;

	  /* honouring reserved labels (ie. "next"). Then resolving unknown labels */
	  if (!strcmp(ptr->jeq.label, "next")) {
	    ptr->jeq.ptr = ptr+1;
	    label_solved = TRUE;
	  }
	  else {
	    for (ptr2 = ptr+1, index = x+1; index < t->ipv4_num; ptr2++, index++) {
	      if (!strcmp(ptr->jeq.label, ptr2->label)) {
	        ptr->jeq.ptr = ptr2;
	        label_solved = TRUE;
	      }
	    }
	  }
	  if (!label_solved) {
	    ptr->jeq.ptr = NULL;
	    Log(LOG_ERR, "ERROR ( %s/%s ): Unresolved label '%s' in map '%s'. Ignoring it.\n",
			config.name, config.type, ptr->jeq.label, filename);
	  }
	  free(ptr->jeq.label);
	  ptr->jeq.label = NULL;
        }
      }

#if defined ENABLE_IPV6
      for (ptr = t->ipv6_base, x = 0; x < t->ipv6_num; ptr++, x++) {
        if (ptr->jeq.label) {
          label_solved = FALSE;
          for (ptr2 = ptr+1, index = x+1; index < t->ipv6_num; ptr2++, index++) {
            if (!strcmp(ptr->jeq.label, ptr2->label)) {
              ptr->jeq.ptr = ptr2;
              label_solved = TRUE;
            }
          }
          if (!label_solved) {
            ptr->jeq.ptr = NULL;
            Log(LOG_ERR, "ERROR ( %s/%s ): Unresolved label '%s' in map '%s'. Ignoring it.\n",
			config.name, config.type, ptr->jeq.label, filename);
          }
          free(ptr->jeq.label);
          ptr->jeq.label = NULL;
        }
      }
#endif

      /* pre_tag_map indexing here */
      if (config.pre_tag_map_index &&
	  (acct_type == ACCT_NF || acct_type == ACCT_SF || acct_type == ACCT_PM)) {
	pt_bitmap_t idx_bmap;

#if defined ENABLE_IPV6
        for (ptr = t->ipv4_base, x = 0; x < MAX(t->ipv4_num, t->ipv6_num); ptr++, x++) {
#else
        for (ptr = t->ipv4_base, x = 0; x < t->ipv4_num; ptr++, x++) {
#endif
	  idx_bmap = pretag_index_build_bitmap(ptr, acct_type);
	  if (!idx_bmap) continue;

	  /* insert bitmap to index list and determine entries per index */ 
	  if (pretag_index_insert_bitmap(t, idx_bmap)) {
	    Log(LOG_WARNING, "WARN ( %s/%s ): Out of indexes for table '%s'. Indexing disabled.\n",
		config.name, config.type, filename);
	    pretag_index_destroy(t, filename);
	    break;
	  }
	}

	/* set handlers */
	pretag_index_set_handlers(t, filename);

	/* allocate indexes */
        pretag_index_allocate(t, filename);

#if defined ENABLE_IPV6
        for (ptr = t->ipv4_base, x = 0; x < MAX(t->ipv4_num, t->ipv6_num); ptr++, x++) {
#else
        for (ptr = t->ipv4_base, x = 0; x < t->ipv4_num; ptr++, x++) {
#endif
          idx_bmap = pretag_index_build_bitmap(ptr, acct_type);
          if (!idx_bmap) continue;

	  /* fill indexes */
	  pretag_index_fill(t, idx_bmap, ptr, filename);
	}
      }
    }
  }

  if (tmp.e) free(tmp.e) ;
  Log(LOG_INFO, "INFO ( %s/%s ): map '%s' successfully (re)loaded.\n", config.name, config.type, filename);

  return;

  handle_error:
  if (*map_allocated && tmp.e) free(tmp.e) ;
  if (t && t->timestamp) {
    Log(LOG_WARNING, "WARN ( %s/%s ): Rolling back the old map '%s'.\n", config.name, config.type, filename);

    /* we update the timestamp to avoid loops */
    stat(filename, &st);
    t->timestamp = st.st_mtime;
  }
  else exit_all(1);
}

u_int8_t pt_check_neg(char **value)
{
  if (**value == '-') {
    (*value)++;
    return TRUE;
  }
  else return FALSE;
}

char *pt_check_range(char *str)
{
  char *ptr;

  if (ptr = strchr(str, '-')) {
    *ptr = '\0';
    ptr++;
    return ptr;
  }
  else return NULL;
}

void pretag_init_vars(struct packet_ptrs *pptrs, struct id_table *t)
{
  if (t->type == ACCT_NF) memset(&pptrs->set_tos, 0, sizeof(s_uint8_t));
  if (t->type == MAP_BGP_TO_XFLOW_AGENT) memset(&pptrs->lookup_bgp_port, 0, sizeof(s_uint16_t));
}

pt_bitmap_t pretag_index_build_bitmap(struct id_entry *ptr, int acct_type)
{
  pt_bitmap_t idx_bmap = 0;
  u_int32_t iterator = 0;

  for (; ptr->func[iterator]; iterator++) idx_bmap |= ptr->func_type[iterator];

  /* 1) invalidate bitmap if we have fields incompatible with indexing */
  if (idx_bmap & PRETAG_FILTER) return 0;

  /* 2) scrub bitmap: remove PRETAG_SET_* fields from the bitmap */
  if (idx_bmap & PRETAG_SET_TOS) idx_bmap ^= PRETAG_SET_TOS;
  if (idx_bmap & PRETAG_SET_TAG) idx_bmap ^= PRETAG_SET_TAG;
  if (idx_bmap & PRETAG_SET_TAG2) idx_bmap ^= PRETAG_SET_TAG2;

  /* 3) add 'ip' to bitmap, if mandated by the map type */
  if (acct_type == ACCT_NF || acct_type == ACCT_SF) idx_bmap |= PRETAG_IP;

  return idx_bmap;
}

int pretag_index_insert_bitmap(struct id_table *t, pt_bitmap_t idx_bmap)
{
  u_int32_t iterator = 0;

  if (!t) return TRUE;

  for (iterator = 0; iterator < MAX_ID_TABLE_INDEXES; iterator++) {
    if (!t->index[iterator].bitmap || t->index[iterator].bitmap == idx_bmap) {
      t->index[iterator].bitmap = idx_bmap;
      t->index[iterator].entries++;
      return FALSE;
    }
  }

  return TRUE;
}

int pretag_index_set_handlers(struct id_table *t, char *filename)
{
  pt_bitmap_t residual_idx_bmap = 0;
  u_int32_t index = 0, iterator = 0, handler_index = 0;

  if (!t) return TRUE;
  
  for (iterator = 0; iterator < MAX_ID_TABLE_INDEXES; iterator++) {
    residual_idx_bmap = t->index[iterator].bitmap;

    handler_index = 0;
    memset(t->index[iterator].idt_handler, 0, sizeof(t->index[iterator].idt_handler));
    memset(t->index[iterator].fdata_handler, 0, sizeof(t->index[iterator].fdata_handler));

    for (index = 0; tag_map_index_entries_dictionary[index].key; index++) {
      if (t->index[iterator].bitmap & tag_map_index_entries_dictionary[index].key) {
        t->index[iterator].idt_handler[handler_index] = (*tag_map_index_entries_dictionary[index].func);
	handler_index++;

        residual_idx_bmap ^= tag_map_index_entries_dictionary[index].key;
      }
    }

    /* we set foreign data handlers here but skip on the residual_idx_bmap */
    for (index = 0; tag_map_index_fdata_dictionary[index].key; index++) {
      if (t->index[iterator].bitmap & tag_map_index_fdata_dictionary[index].key) {
        t->index[iterator].fdata_handler[handler_index] = (*tag_map_index_fdata_dictionary[index].func);
        handler_index++;
      }
    }

    if (residual_idx_bmap) {
      Log(LOG_WARNING, "WARN ( %s/%s ): pre_tag_map_index: not supported for field(s) %x in table '%s'. Indexing disabled.\n",
		config.name, config.type, residual_idx_bmap, filename);
      pretag_index_destroy(t, filename);
    }
  }

  return FALSE;
}

int pretag_index_allocate(struct id_table *t, char *filename)
{
  pt_bitmap_t idx_t_size = 0;
  u_int32_t iterator = 0;

  if (!t) return TRUE;

  for (iterator = 0; iterator < MAX_ID_TABLE_INDEXES; iterator++) {
    if (t->index[iterator].bitmap) {
      Log(LOG_DEBUG, "DEBUG ( %s/%s ): pre_tag_map_index: index bitmap %x (%u entries) for table '%s'\n", config.name,
    		config.type, t->index[iterator].bitmap, t->index[iterator].entries, filename);

      assert(!t->index[iterator].idx_t);
      idx_t_size = IDT_INDEX_HASH_BASE(t->index[iterator].entries) * sizeof(struct id_index_entry);
      t->index[iterator].idx_t = malloc(idx_t_size);

      if (!t->index[iterator].idx_t) {
        Log(LOG_ERR, "ERROR ( %s/%s ): pre_tag_map_index: unable to allocate index %x for table '%s'\n", config.name,
		config.type, t->index[iterator].bitmap, filename);
	t->index[iterator].bitmap = 0;
	t->index[iterator].entries = 0;
      }
      else memset(t->index[iterator].idx_t, 0, idx_t_size); 
    }
  }

  return FALSE;
}

int pretag_index_fill(struct id_table *t, pt_bitmap_t idx_bmap, struct id_entry *ptr, char *filename)
{
  struct id_entry e;
  u_int32_t index = 0, iterator = 0, handler_index = 0;

  if (!t) return TRUE;

  for (iterator = 0; iterator < MAX_ID_TABLE_INDEXES; iterator++) {
    if (t->index[iterator].bitmap && t->index[iterator].bitmap == idx_bmap) {
      struct id_index_entry *idie;
      int modulo;

      /* fill 'e' in and compute modulo */
      memset(&e, 0, sizeof(struct id_entry));
      for (handler_index = 0; t->index[iterator].idt_handler[handler_index]; handler_index++) {
	(*t->index[iterator].idt_handler[handler_index])(&e, ptr);
      }
      modulo = cache_crc32((unsigned char *)&e, sizeof(struct id_entry)) % IDT_INDEX_HASH_BASE(t->index[iterator].entries);
      idie = &t->index[iterator].idx_t[modulo];

      for (index = 0; index < ID_TABLE_INDEX_DEPTH; index++) {
        if (!idie->e[index]) {
          idie->e[index] = ptr;
          break;
        }
      }

      if (index == ID_TABLE_INDEX_DEPTH) {
        Log(LOG_WARNING, "WARN ( %s/%s ): pre_tag_map_index: out of index space %x for table '%s'. Indexing disabled.\n",
		config.name, config.type, idx_bmap, filename);
	pretag_index_destroy(t, filename);
	break;
      }
    }
  }

  return FALSE;
}

void pretag_index_destroy(struct id_table *t, char *filename)
{
  u_int32_t iterator = 0;

  if (!t) return;

  for (iterator = 0; iterator < MAX_ID_TABLE_INDEXES; iterator++) {
    if (t->index[iterator].idx_t) {
      free(t->index[iterator].idx_t);
      Log(LOG_INFO, "INFO ( %s/%s ): pre_tag_map_index: destroyed index %x for table '%s'.\n",
		config.name, config.type, t->index[iterator].bitmap, filename);
    }
    memset(&t->index[iterator], 0, sizeof(struct id_table_index));
  }
}

void pretag_index_lookup(struct id_table *t, struct packet_ptrs *pptrs, struct id_entry **index_results)
{
  struct id_entry res_idt, res_fdata;
  struct id_index_entry *idie;
  u_int32_t iterator, index_cc, index_hdlr;
  int modulo;

  if (!t || !index_results) return;

  memset(&index_results, 0, (sizeof(struct id_entry) * MAX_ID_TABLE_INDEXES));
  memset(&res_idt, 0, sizeof(res_idt));
  memset(&res_fdata, 0, sizeof(res_fdata));

  for (iterator = 0; iterator < MAX_ID_TABLE_INDEXES; iterator++) {
    for (index_hdlr = 0; (*t->index[iterator].fdata_handler[index_hdlr]); index_hdlr++) {
      (*t->index[iterator].fdata_handler[index_hdlr])(&res_fdata, pptrs);
    }

    modulo = cache_crc32((unsigned char *)&res_fdata, sizeof(struct id_entry)) % IDT_INDEX_HASH_BASE(t->index[iterator].entries);
    idie = &t->index[iterator].idx_t[modulo];

    for (index_cc = 0; idie->e[index_cc] && index_cc < ID_TABLE_INDEX_DEPTH; index_cc++) {
      for (index_hdlr = 0; (*t->index[iterator].idt_handler[index_hdlr]); index_hdlr++) {
        (*t->index[iterator].idt_handler[index_hdlr])(&res_idt, idie->e[index_cc]);
      }

      if (!memcmp(&res_idt, &res_fdata, sizeof(struct id_entry))) {
        index_results[iterator] = idie->e[index_cc];
        break;
      }
    }
  }

  pretag_index_results_sort(index_results);
}

void pretag_index_results_sort(struct id_entry **index_results)
{
  struct id_entry *ptr = NULL;
  u_int32_t i, j;

  if (!index_results) return;

  for (i = 0, j = 1; index_results[i] && i < ID_TABLE_INDEX_DEPTH; i++, j++) {
    if (index_results[j] && j < ID_TABLE_INDEX_DEPTH) {
      if (index_results[i]->pos > index_results[j]->pos) {
	ptr = index_results[j];
	index_results[j] = index_results[i];
	index_results[i] = ptr;
      }
    }
  }
}
