# /usr/bin/env python3

#
# Copyright (C) 2006-2020 by The Odamex Team.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#

import json
import os
import pathlib
import sys

from b2sdk.v1 import InMemoryAccountInfo
from b2sdk.v1 import B2Api

B2_APP_KEY = os.getenv("B2_APP_KEY")
B2_BUCKET_ID = os.getenv("B2_BUCKET_ID")
B2_KEY_ID = os.getenv("B2_KEY_ID")

if B2_APP_KEY is None:
    print("==> B2 credentials are absent, skipping...")
    sys.exit(0)

GITHUB_EVENT_NAME = os.getenv("GITHUB_EVENT_NAME")
GITHUB_SHA = os.getenv("GITHUB_SHA")
GITHUB_REF = os.getenv("GITHUB_REF")

with open(os.getenv("GITHUB_EVENT_PATH")) as fh:
    event_data = json.load(fh)

if GITHUB_EVENT_NAME == "push":
    # Commit name
    MESSAGE = event_data['commits'][0]['message']
elif GITHUB_EVENT_NAME == "pull_request":
    # PR name
    MESSAGE = event_data['pull_request']['body']
else:
    MESSAGE = ""

if __name__ == "__main__":
    src_dir = sys.argv[1]
    dest_dir = sys.argv[2]

    info = InMemoryAccountInfo()
    b2_api = B2Api(info)
    b2_api.authorize_account("production", B2_KEY_ID, B2_APP_KEY)
    bucket = b2_api.get_bucket_by_id(B2_BUCKET_ID)

    # Iterate every file in the directory.
    for filename in pathlib.Path(src_dir).iterdir():
        print(f"==> Uploading {filename}...")
        bucket.upload_local_file(
            filename,
            dest_dir + "/" + filename.name,
            content_type="application/zip",
            file_infos={
                "platform": dest_dir,
                "event_name": GITHUB_EVENT_NAME,
                "sha": GITHUB_SHA,
                "ref": GITHUB_REF,
                "message": MESSAGE,
            },
        )
