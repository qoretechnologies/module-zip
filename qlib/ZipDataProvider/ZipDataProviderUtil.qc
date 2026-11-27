# -*- mode: qore; indent-tabs-mode: nil -*-
#! Qore ZipDataProvider utility functions

/** ZipDataProviderUtil.qc Copyright (C) 2026 Qore Technologies, s.r.o.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#! Contains all public definitions in the ZipDataProvider module
public namespace ZipDataProvider {
    #! Refuses a filesystem path given in a request if the calling code is restricted from the file system
    /** Library code runs with its own %Program's options, so a path that the calling code gives is refused before it
        is used if that code is restricted from the \c FILESYSTEM functional domain (for example with
        \c PO_NO_FILESYSTEM), also when the provider was lent to that code by the host (see
        @ref Qore::SandboxManager::isLentToRestricted() "SandboxManager::isLentToRestricted()").

        @param field the name of the request field, for the error message
        @param path the path; no check is made if it is not set (any value that is set is checked, also one that
        evaluates to @ref False, such as \c "0", which some actions use as a path)

        @throw PERMISSION-DENIED the calling code is restricted from the \c FILESYSTEM functional domain
    */
    sub zip_check_caller_path(string field, auto path) {
        if (exists path && SandboxManager::isLentToRestricted(DOM_FILESYSTEM)) {
            throw "PERMISSION-DENIED", sprintf("the %y field of the request cannot be used: the calling code is "
                "restricted from the FILESYSTEM functional domain", field);
        }
    }

    #! Refuses the filesystem paths of the entries of a request if the calling code is restricted from the file system
    /** @param field the name of the request field with the entries, for the error message
        @param entries the entries; the \c path of an entry is a filesystem path that the actions read if the entry
        has no \c data

        @throw PERMISSION-DENIED the calling code is restricted from the \c FILESYSTEM functional domain
    */
    sub zip_check_caller_entry_paths(string field, auto entries) {
        if (entries.typeCode() != NT_LIST) {
            return;
        }
        foreach auto entry in (entries) {
            if (entry.typeCode() == NT_HASH && !entry.data) {
                zip_check_caller_path(sprintf("%s[%d].path", field, $#), entry.path);
            }
        }
    }

    #! Refuses a location given in a request if the calling code is restricted from a functional domain of its handler
    /** The handler of the location's scheme decides the functional domains (see
        @ref FileLocationHandler::AbstractFileLocationHandler::getCallerDomains()
        "AbstractFileLocationHandler::getCallerDomains()"), as when the location is read or written; the calling code
        is judged as by @ref Qore::SandboxManager::isLentToRestricted() "SandboxManager::isLentToRestricted()", so the
        code that a host-created provider was lent to cannot give a location that it could not use itself.

        @param field the name of the request field, for the error message
        @param location the location; no check is made if it is not set

        @throw LOCATION-ERROR the options of the location cannot be parsed
        @throw PERMISSION-DENIED the calling code is restricted from a functional domain of the location's handler
    */
    sub zip_check_caller_location(string field, auto location) {
        if (!exists location) {
            return;
        }
        string loc = location;
        string scheme = (loc =~ x/^(\w+):\/\//)[0] ?? "file";
        AbstractFileLocationHandler handler;
        try {
            handler = FileLocationHandler::getHandler(scheme);
        } catch (hash<ExceptionInfo> ex) {
            if (ex.err == "SCHEME-ERROR") {
                # an unknown scheme is refused when the location is used
                return;
            }
            rethrow;
        }
        # the options at the end of the location, as FileLocationHandler takes them
        *hash<auto> opts;
        *string opt_str = (loc =~ x/((?:{).*(?:}))$/)[0];
        if (opt_str) {
            auto v = parse_to_qore_value(opt_str);
            if (v.typeCode() == NT_HASH) {
                opts = v;
            } else if (v.strp()) {
                opts = {v: True};
            } else {
                throw "LOCATION-ERROR", sprintf("the options of the location in the %y field of the request cannot be "
                    "parsed", field);
            }
        }
        int dom = handler.getCallerDomains(scheme, opts);
        if (dom && SandboxManager::isLentToRestricted(dom)) {
            throw "PERMISSION-DENIED", sprintf("the %y field of the request cannot be used: the calling code is "
                "restricted from a functional domain of the handler of %y locations", field, scheme);
        }
        # the handler of conn locations has no domains of its own and gives the options that the caller adds to the
        # handler that it resolves the location to, which judges the calling code as
        # SandboxManager::isCallerRestricted() does; for the code that a host-created provider was lent to, which that
        # does not see, the options are refused here if that code is restricted from the file system or the network
        if (!dom && opts && (scheme == "conn" || scheme == "connection")
                && ((SandboxManager::isLentToRestricted(DOM_FILESYSTEM)
                        && !SandboxManager::isCallerRestricted(DOM_FILESYSTEM))
                    || (SandboxManager::isLentToRestricted(DOM_NETWORK)
                        && !SandboxManager::isCallerRestricted(DOM_NETWORK)))) {
            throw "PERMISSION-DENIED", sprintf("the options of the location in the %y field of the request cannot be "
                "used: the calling code is restricted from the FILESYSTEM or NETWORK functional domain", field);
        }
    }

    #! Check if a name matches a glob pattern (using Util::glob_to_regex())
    public bool sub glob_match(string pattern, string name) {
        return regex(name, glob_to_regex(pattern));
    }

    #! Filter a list of entry info hashes by include/exclude patterns and regex
    /** Include filters are applied first (whitelist), then exclude filters (blacklist).
        Blacklist overrides whitelist. If no include filter is set, all entries are included
        by default before exclude filtering.
    */
    public list<hash<ZipEntryInfo>> sub filter_entries(
        list<hash<ZipEntryInfo>> entries,
        *list<string> include_patterns,
        *list<string> exclude_patterns,
        *string include_regex,
        *string exclude_regex,
    ) {
        if (!include_patterns && !exclude_patterns && !include_regex && !exclude_regex) {
            return entries;
        }

        # Pre-compile glob patterns to regex for performance
        *list<string> include_regexes;
        if (include_patterns) {
            include_regexes = map glob_to_regex($1), include_patterns;
        }
        *list<string> exclude_regexes;
        if (exclude_patterns) {
            exclude_regexes = map glob_to_regex($1), exclude_patterns;
        }

        list<hash<ZipEntryInfo>> result = ();
        foreach hash<ZipEntryInfo> entry in (entries) {
            string name = entry.name;

            # Include check: entry must match at least one include pattern/regex
            if (include_regexes || include_regex) {
                bool included = False;
                if (include_regexes) {
                    foreach string pat_regex in (include_regexes) {
                        if (regex(name, pat_regex)) {
                            included = True;
                            break;
                        }
                    }
                }
                if (!included && include_regex) {
                    included = regex(name, include_regex);
                }
                if (!included) {
                    continue;
                }
            }

            # Exclude check: entry must NOT match any exclude pattern/regex
            if (exclude_regexes) {
                bool excluded = False;
                foreach string pat_regex in (exclude_regexes) {
                    if (regex(name, pat_regex)) {
                        excluded = True;
                        break;
                    }
                }
                if (excluded) {
                    continue;
                }
            }
            if (exclude_regex && regex(name, exclude_regex)) {
                continue;
            }

            result += entry;
        }
        return result;
    }
}
