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
