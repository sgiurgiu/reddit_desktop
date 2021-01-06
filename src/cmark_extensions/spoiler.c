#include "spoiler.h"
#include <parser.h>

cmark_node_type CMARK_NODE_SPOILER;

static cmark_node *match(cmark_syntax_extension *self, cmark_parser *parser,
                         cmark_node *parent, unsigned char character,
                         cmark_inline_parser *inline_parser) {
  cmark_node *res = NULL;
  int left_flanking, right_flanking, punct_before, punct_after, delims;
  char buffer[101];

  if (character != '>' && character != '!')
    return NULL;

  if(character == '!')
  {
      int previous_offset = cmark_inline_parser_get_offset(inline_parser)-1;
      if(previous_offset < 0) return NULL;
      unsigned char c = cmark_inline_parser_peek_at(inline_parser,previous_offset);
      if(c != '>') return NULL;
  }

  delims = cmark_inline_parser_scan_delimiters(
      inline_parser, sizeof(buffer) - 1, '<',
      &left_flanking,
      &right_flanking, &punct_before, &punct_after);

  memset(buffer, '~', delims);
  buffer[delims] = 0;

  res = cmark_node_new_with_mem(CMARK_NODE_TEXT, parser->mem);
  cmark_node_set_literal(res, buffer);
  res->start_line = res->end_line = cmark_inline_parser_get_line(inline_parser);
  res->start_column = cmark_inline_parser_get_column(inline_parser) - delims;

  if ((left_flanking || right_flanking)) {
    cmark_inline_parser_push_delimiter(inline_parser, character, left_flanking,
                                       right_flanking, res);
  }

  return res;
}

static delimiter *insert(cmark_syntax_extension *self, cmark_parser *parser,
                         cmark_inline_parser *inline_parser, delimiter *opener,
                         delimiter *closer) {
  cmark_node *strikethrough;
  cmark_node *tmp, *next;
  delimiter *delim, *tmp_delim;
  delimiter *res = closer->next;

  strikethrough = opener->inl_text;

  if (opener->inl_text->as.literal.len != closer->inl_text->as.literal.len)
    goto done;

  if (!cmark_node_set_type(strikethrough, CMARK_NODE_SPOILER))
    goto done;

  cmark_node_set_syntax_extension(strikethrough, self);

  tmp = cmark_node_next(opener->inl_text);

  while (tmp) {
    if (tmp == closer->inl_text)
      break;
    next = cmark_node_next(tmp);
    cmark_node_append_child(strikethrough, tmp);
    tmp = next;
  }

  strikethrough->end_column = closer->inl_text->start_column + closer->inl_text->as.literal.len - 1;
  cmark_node_free(closer->inl_text);

  delim = closer;
  while (delim != NULL && delim != opener) {
    tmp_delim = delim->previous;
    cmark_inline_parser_remove_delimiter(inline_parser, delim);
    delim = tmp_delim;
  }

  cmark_inline_parser_remove_delimiter(inline_parser, opener);

done:
  return res;
}

static const char *get_type_string(cmark_syntax_extension *extension,
                                   cmark_node *node) {
  return node->type == CMARK_NODE_SPOILER ? "spoiler" : "<unknown>";
}

static int can_contain(cmark_syntax_extension *extension, cmark_node *node,
                       cmark_node_type child_type) {
    if (node->type != CMARK_NODE_SPOILER)
      return false;

    return CMARK_NODE_TYPE_INLINE_P(child_type);
}

cmark_syntax_extension* create_spoiler_extension(void)
{
    cmark_syntax_extension *ext = cmark_syntax_extension_new("spoiler");
    cmark_llist *special_chars = NULL;

    cmark_syntax_extension_set_get_type_string_func(ext, get_type_string);
    cmark_syntax_extension_set_can_contain_func(ext, can_contain);

    CMARK_NODE_SPOILER = cmark_syntax_extension_add_node(1);

    cmark_syntax_extension_set_match_inline_func(ext, match);
    cmark_syntax_extension_set_inline_from_delim_func(ext, insert);

    cmark_mem *mem = cmark_get_default_mem_allocator();
    special_chars = cmark_llist_append(mem, special_chars, (void *)'>');        
    cmark_llist_append(mem, special_chars, (void *)'!');
    cmark_llist_append(mem, special_chars, (void *)'<');

    cmark_syntax_extension_set_special_inline_chars(ext, special_chars);

    cmark_syntax_extension_set_emphasis(ext, 1);

    return ext;
}
