#!/bin/bash
cd "$(dirname "$0")"
bundle config set --local path vendor/bundle
bundle install
bundle exec jekyll serve
