
import 'package:lk/ui.dart';

class ParseResult {
    Object o = null;
}

enum PathMode {
    none,
    moveTo,
    lineTo,
    horizontalLineTo,
    verticalLineTo,
    cubicTo,
}

enum Error {
    eos,
    attrFormat,
}

abstract class XmlNode extends ParseResult {
    int parseAttribute(String attribute, String str, int start) {
        return -1;
    }

    int parseElement(String element, String str, int start) {
        return -1;
    }
}

class VectorParser {

    static const bool debug = false;

    static const int _linefeed = 10;
    static const int _space = 32;
    static const int _quotes = 34;
    static const int _hash = 35;
    static const int _comma = 44;
    static const int _minus = 45;
    static const int _dot = 46;
    static const int _slash = 47;
    static const int _zero = 48;
    static const int _nine = 57;
    static const int _open_bracket = 60;
    static const int _equals = 61;
    static const int _close_bracket = 62;
    static const int _question = 63;
    static const int _A = 64;
    static const int _C = 67;
    static const int _F = 70;
    static const int _H = 72;
    static const int _L = 76;
    static const int _M = 77;
    static const int _V = 86;
    static const int _Z = 90;
    static const int _a = 96;
    static const int _c = 99;
    static const int _f = 102;
    static const int _h = 104;
    static const int _l = 108;
    static const int _m = 109;
    static const int _v = 118;
    static const int _z = 122;

    static bool _isspace(int c) {
        return (c == _space) || (c == _linefeed);
    }

    static int error(Error e) {
        switch (e) {
            case Error.eos:
                print("Unexpected end of string");
                break;
            case Error.attrFormat:
                print("Incorrect attribute foramt");
                break;
        }
        return -1;
    }

    static int parseAttributeStart(String str, int start) {
        bool got_equals = false;

        for (;;) {
            if (start >= str.length)
                return error(Error.eos);
            int c = str.codeUnits[start];
            switch (c) {
            case _close_bracket:
                return -1;
            case _quotes:
                if (!got_equals) {
                    print("Unexpected quotes");
                    return -1;
                }
                start++;
                if (start >= str.length)
                    return error(Error.eos);
                return start;
            case _equals:
                if (got_equals) {
                    print("Unexpected equals");
                    return -1;
                }
                got_equals = true;
                break;
            default:
                if (!_isspace(c)) {
                    print("Unexpected while parsing attribute start ${str[start]}");
                    return -1;
                }
            }
            start++;
        }

        assert(false);
        return -1;
    }

    static int parseAttributeInteger(String str, int start, ParseResult result) {

        start = parseAttributeStart(str, start);
        if (start < 0)
            return start;

        int c = str.codeUnits[start];
        if (c != _hash && c != _minus && (c < _zero || c > _nine)) {
            print("parseAttributeInteger unexpected start ${str[start]}");
            return -1;
        }

        int base = 10;
        if (c == _hash) {
            base = 16;
            start++;
            if (start >= str.length)
                return error(Error.eos);
        }

        int end = start + 1;

        for (;;) {
            if (end >= str.length) {
                print("Unexpected end of string");
                return -1;
            }
            c = str.codeUnits[end];
            if (c >= _zero && c <= _nine) {
                // valid
            } else if (base == 16 &&
                (c >= _A && c <= _F) || (c >= _a && c <= _f)) {
                // valid
            } else {
                break;
            }
            end++;
        }

        String istr = str.substring(start, end);
        int i = int.parse(istr, radix: base);
        if (debug) 
            print("got int $i");

        if (c != _quotes)
            return error(Error.attrFormat);

        result.o = i;
        return end;
    }

    static int parseDouble(String str, int start, ParseResult result) {
        if (debug) 
            print("parseDouble");

        for (;;) {
            if (start >= str.length)
                return -1;
            int c = str.codeUnits[start];
            if (c == _close_bracket)
                return -1;
            if (c != _comma && !_isspace(c))
                break;
            start++;
        }

        int c = str.codeUnits[start];
        if (c != _minus && (c < _zero || c > _nine)) {
            if (debug) 
                print("parseDouble unexpected ${str[start]}");
            return -1;
        }

        int end = start + 1;
        bool found_decimal = false;

        for (;;) {
            if (end >= str.length)
                return -1;
            c = str.codeUnits[end];
            if (!found_decimal && c == _dot) {
                found_decimal = true;
            } else if (c < _zero || c > _nine) {
                break;
            }
            end++;
        }

        String fstr = str.substring(start, end);
        double f = double.parse(fstr);
        if (debug) 
            print("got double $f");

        result.o = f;
        return end - 1;
    }

    static int parseAttributePathData(String str, int start, Path path) {

        for (;;) {
            if (start >= str.length)
                return -1;
            int c = str.codeUnits[start];
            if (c == _close_bracket)
                return -1;
            if (c == _quotes)
                break;
            start++;
        }

        start++;
        int end = start;

        ParseResult working = new ParseResult();
        PathMode mode = PathMode.none;
        bool relative = false;
        double x = 0.0, y = 0.0; // the current point

        for (;;) {
            if (end >= str.length)
                return -1;
            int c = str.codeUnits[end];
            if (c == _minus || (c >= _zero && c <= _nine)) {
                if (mode == PathMode.none) {
                    print("got digit but mode is none");
                    return -1;
                }
                end = VectorParser.parseDouble(str, end, working);
                if (end < 0)
                    return end;
                if (mode == PathMode.horizontalLineTo) {
                    x = relative ? x + working.o : working.o;
                    if (debug) 
                        print("horizontalLineTo $x $y");
                    path.lineTo(x, y);
                } else if (mode == PathMode.verticalLineTo) {
                    y = relative ? y + working.o : working.o;
                    if (debug) 
                        print("verticalLineTo $x $y");
                    path.lineTo(x, y);
                } else if (mode == PathMode.moveTo) {
                    x = relative ? x + working.o : working.o;
                    end = VectorParser.parseDouble(str, end + 1, working);
                    if (end < 0)
                        return end;
                    y = relative ? y + working.o : working.o;
                    if (debug) 
                        print("path moveto $x $y");
                    path.moveTo(x, y);
                } else if (mode == PathMode.lineTo) {
                    x = relative ? x + working.o : working.o;
                    end = VectorParser.parseDouble(str, end + 1, working);
                    if (end < 0)
                        return end;
                    y = relative ? y + working.o : working.o;
                    if (debug) 
                        print("path lineto $x $y");
                    path.lineTo(x, y);
                } else {
                    double x1 = relative ? x + working.o : working.o;
                    end = VectorParser.parseDouble(str, end + 1, working);
                    if (end < 0)
                        return end;
                    double y1 = relative ? y + working.o : working.o;
                    end = VectorParser.parseDouble(str, end + 1, working);
                    if (end < 0)
                        return end;
                    double x2 = relative ? x + working.o : working.o;
                    end = VectorParser.parseDouble(str, end + 1, working);
                    if (end < 0)
                        return end;
                    double y2 = relative ? y + working.o : working.o;
                    end = VectorParser.parseDouble(str, end + 1, working);
                    if (end < 0)
                        return end;
                    x = relative ? x + working.o : working.o;
                    end = VectorParser.parseDouble(str, end + 1, working);
                    if (end < 0)
                        return end;
                    y = relative ? y + working.o : working.o;
                    if (debug) 
                        print("cubicTo $x1 $y1 $x2 $y2 $x $y");
                    path.cubicTo(x1, y1, x2, y2, x, y);
                }
            } else switch (c) {
                case _m:
                case _M:
                    mode = PathMode.moveTo;
                    relative = (c == _m);
                    break;
                case _l:
                case _L:
                    mode = PathMode.lineTo;
                    relative = (c == _l);
                    break;
                case _c:
                case _C:
                    mode = PathMode.cubicTo;
                    relative = (c == _c);
                    break;
                case _h:
                case _H:
                    mode = PathMode.horizontalLineTo;
                    relative = (c == _h);
                    break;
                case _v:
                case _V:
                    mode = PathMode.verticalLineTo;
                    relative = (c == _v);
                    break;
                case _z:
                case _Z:
                    if (debug) 
                        print("path close");
                    path.close();
                    break;
                case _close_bracket:
                    return -1;
                case _quotes:
                    return end;
                default:
                    if (!_isspace(c)) {
                        print("unexpected path data input ${str[end]}");
                        return -1;
                    }
            }
            end++;
        }

        assert(false);
        return -1;
    }

    static int parseElement(XmlNode node, String str, int start) {

        if (debug) 
            print("parseElement from $start");

        // Loop over attributes
        for (bool moreattrs = true; moreattrs;) {

            // find the start
            for (;;) {
                if (start >= str.length)
                    return -1;
                int c = str.codeUnits[start];
                if (c == _slash) {
                    if (start + 1 == str.length || str.codeUnits[start + 1] != _close_bracket)
                        return -1;
                    return start + 1;
                } else if (c == _close_bracket) {
                    moreattrs = false;
                    break;
                } else if (!_isspace(c)) {
                    break;
                }
                start++;
            }

            // find the end
            int end = start;

            for (;;) {
                if (end == str.length)
                    return -1;
                int c = str.codeUnits[end];
                if (c == _slash) {
                    if (end + 1 == str.length || str.codeUnits[end + 1] != _close_bracket)
                        return -1;
                    return end + 1;
                } else if (c == _close_bracket) {
                    moreattrs = false;
                    break;
                } else if (_isspace(c) || c == _equals) {
                    break;
                }
                end++;
            }

            if (moreattrs) {
                String attr = str.substring(start, end);
                if (debug) 
                    print("parser got attr $attr");
                end = node.parseAttribute(attr, str, end);
                if (debug) 
                    print("parseAttribute returned $end");

                if (end < 0)
                    return end;
            }

            start = end + 1;
        }

        return parseChildren(node, str, start);
    }

    static int parseChildren(XmlNode node, String str, int start) {
        if (debug) 
            print("parseChildren from $start");

        int end;

        // Loop over elements
        for (bool moreelements = true; moreelements; ) {

            for (;;) {
                if (start >= str.length)
                    return str.length;
                int c = str.codeUnits[start];
                if (!_isspace(c))
                    break;
                start++;
            }

            if (str.codeUnits[start] != _open_bracket) {
                print("expected open bracket, found ${str[start]}");
                return -1;
            }

            start++;
            if (start == str.length)
                return -1;

            moreelements = (str.codeUnits[start] != _slash);

            end = start;

            for (;;) {
                if (end == str.length)
                    return -1;
                int c = str.codeUnits[end];
                if (_isspace(c) || (c == _close_bracket))
                    break;
                end++;
            }

            String tag = str.substring(start, end);
            if (debug) 
                print("parser got element tag $tag");
            end = node.parseElement(tag, str, end);
            if (debug)
                print("parseElement returned $end");

            if (end < 0)
                return end;

            start = end + 1;
        }

        if (debug)
            print("parseChildren returning $end");
        return end;
    }

    static int parseUnknown(String str, int start) {
        if (debug)
            print("parseUnknown at $start");
        if (start >= str.length)
            return -1;
        int end = start;
        for (;;) {
            int c = str.codeUnits[end];
            if (c == _slash || c == _question)
                break;
            end++;
            if (end == str.length)
                return -1;
        }
        end++;
        if (end == str.length)
            return -1;
        if (str.codeUnits[end] != _close_bracket)
            return -1;
        String unknown = str.substring(start, end + 1);
        if (debug)
            print("parsed unknown $unknown");
        return end;
    }

    static int parseUnknownAttr(String str, int start) {
        if (debug) 
            print("parseUnknownAttr at $start");
        if (start >= str.length)
            return -1;
        assert(str.codeUnits[start] == _equals);

        for (;;) {
            if (start == str.length)
                return -1;
            int c = str.codeUnits[start];
            if (c == _close_bracket)
                return -1;
            if (c == _quotes)
                break;
            start++;
        }

        int end = start + 1;

        for (;;) {
            if (end == str.length)
                return -1;
            int c = str.codeUnits[end];
            if (c == _close_bracket)
                return -1;
            if (c == _quotes)
                break;
            end++;
        }

        String unknown = str.substring(start, end + 1);
        if (debug)
            print("parsed unknown attr $unknown");

        return end;
    }

    static VectorNode parse(String str) {
        RootNode root = new RootNode();
        int start = 0;
        int end = VectorParser.parseChildren(root, str, start);
        if (end < 0)
            return null;
        return root.vector;
    }

}

//////////////////////////////////////////////////////////////////////////////

class PathNode extends XmlNode {
    int fillColor;
    Path path;

    int parseAttribute(String attribute, String str, int start) {
        int end = -1;

        if (attribute.compareTo("fillColor") == 0) {
            ParseResult working = new ParseResult();
            end = VectorParser.parseAttributeInteger(str, start, working);
            fillColor = working.o;
            // TODO: handle translucent color
            fillColor |= 0xff000000;
        } else if (attribute.compareTo("pathData") == 0) {
            if (path == null) {
                path = new Path();
            }
            end = VectorParser.parseAttributePathData(str, start, path);
        }

        return end;
    }
}

class GroupNode extends XmlNode {
    List<PathNode> pathList;

    int parseElement(String element, String str, int start) {
        int end = -1;

        if (element.compareTo("/group") == 0) {
            end = start;
        } else if (element.compareTo("path") == 0) {
            if (pathList == null) {
                pathList = new List<PathNode>();
            }
            PathNode path = new PathNode();
            end = VectorParser.parseElement(path, str, start);
            pathList.add(path);
        } else {
            end = VectorParser.parseUnknown(str, start);
        }

        return end;
    }
}

class VectorNode extends XmlNode {
    int viewportWidth;
    int viewportHeight;
    List<GroupNode> groupList;

    int parseAttribute(String attribute, String str, int start) {
        ParseResult working = new ParseResult();
        int end = -1;

        if (attribute.compareTo("viewportWidth") == 0) {
            end = VectorParser.parseAttributeInteger(str, start, working);
            viewportWidth = working.o;
        } else if (attribute.compareTo("viewportHeight") == 0) {
            end = VectorParser.parseAttributeInteger(str, start, working);
            viewportHeight = working.o;
        } else {
            end = VectorParser.parseUnknownAttr(str, start);
        }

        return end;
    }

    int parseElement(String element, String str, int start) {
        int end = -1;

        if (element.compareTo("/vector") == 0) {
            end = start;
        } else if (element.compareTo("group") == 0) {
            if (groupList == null) {
                groupList = new List<GroupNode>();
            }
            GroupNode group = new GroupNode();
            end = VectorParser.parseElement(group, str, start);
            groupList.add(group);
        } else {
            end = VectorParser.parseUnknown(str, start);
        }

        return end;
    }
}

class RootNode extends XmlNode {
    VectorNode vector;

    int parseElement(String element, String str, int start) {
        int end = -1;
        if (element.compareTo("?xml") == 0) {
            end = VectorParser.parseUnknown(str, start);
        } else if (element.compareTo("vector") == 0) {
            if (vector == null) {
                vector = new VectorNode();
            }
            end = VectorParser.parseElement(vector, str, start);
        } else {
            end = VectorParser.parseUnknown(str, start);
        }
        return end;
    }
}
