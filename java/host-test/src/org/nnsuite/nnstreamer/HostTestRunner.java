/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API host test
 * Copyright (C) 2026 Samsung Electronics Co., Ltd.
 */

package org.nnsuite.nnstreamer;

import org.junit.internal.TextListener;
import org.junit.runner.Description;
import org.junit.runner.JUnitCore;
import org.junit.runner.Request;
import org.junit.runner.Result;
import org.junit.runner.manipulation.Filter;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Runs the instrumented test classes on a desktop JVM, skipping the tests listed in an exclude file.
 *
 * Usage: HostTestRunner (exclude file) (test class name)...
 *
 * Each line of the exclude file is a class name, or a class name and a method name joined
 * with '#'. Text after '#' at the start of a line or after whitespace is a comment.
 * An entry that does not match any test fails the run, so that the list stays accurate.
 */
public final class HostTestRunner {
    private static Set<String> readExcludeFile(String path) throws IOException {
        Set<String> entries = new HashSet<>();

        for (String line : Files.readAllLines(Paths.get(path), StandardCharsets.UTF_8)) {
            int comment = line.indexOf(" #");
            String entry = (comment >= 0 ? line.substring(0, comment) : line).trim();

            if (!entry.isEmpty() && !entry.startsWith("#")) {
                entries.add(entry);
            }
        }

        return entries;
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 2) {
            System.err.println("Usage: HostTestRunner <exclude file> <test class>...");
            System.exit(2);
        }

        final Set<String> excluded = readExcludeFile(args[0]);
        final Set<String> matched = new HashSet<>();
        List<Class<?>> classes = new ArrayList<>();

        for (int i = 1; i < args.length; i++) {
            Class<?> klass = Class.forName(args[i]);

            if (excluded.contains(klass.getName())) {
                matched.add(klass.getName());
                System.out.println("Excluded class " + klass.getName());
            } else {
                classes.add(klass);
            }
        }

        Filter filter = new Filter() {
            @Override
            public boolean shouldRun(Description description) {
                if (!description.isTest()) {
                    return true;
                }

                String name = description.getClassName() + "#" + description.getMethodName();

                if (excluded.contains(name)) {
                    matched.add(name);
                    return false;
                }

                return true;
            }

            @Override
            public String describe() {
                return "exclude tests listed in " + args[0];
            }
        };

        JUnitCore core = new JUnitCore();
        core.addListener(new TextListener(System.out));

        Result result = core.run(Request.classes(classes.toArray(new Class<?>[0])).filterWith(filter));
        boolean success = result.wasSuccessful();

        System.out.println("Excluded " + (matched.size()) + " entries.");

        for (String entry : excluded) {
            if (!matched.contains(entry)) {
                System.out.println("Exclude entry does not match any test: " + entry);
                success = false;
            }
        }

        System.exit(success ? 0 : 1);
    }
}
