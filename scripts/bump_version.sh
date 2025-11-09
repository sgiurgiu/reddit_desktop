#!/bin/bash

export

git --version
git remote remove origin
git remote add origin https://${GITLAB_USER_LOGIN}:${ACCESS_TOKEN}@${CI_SERVER_HOST}/${CI_PROJECT_PATH}
git config user.email "gitlab-runner@linuxbuild"
git config user.name "gitlab runner"
cur_version=$(git describe --tags)
if [[ -z ${cur_version} ]]
then
    cur_version="1.0.0"
fi
next_version=$(semver bump patch ${cur_version})
git tag -a ${next_version} -m "Release version ${next_version}"
git push origin --tags

