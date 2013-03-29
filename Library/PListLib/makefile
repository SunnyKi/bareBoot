HDRS = plist.h plist_helpers.h plist_xml_parser.h b64/cencode.h b64/cdecode.h

SRCS = main.c plist_helpers_os.c plist_internal.c plist_xml_out.c plist_xml_parser.c b64/cencode.c b64/cdecode.c

pltest:
	${CC} ${CFLAGS} -Ib64 ${SRCS}
