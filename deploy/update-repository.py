import os
import json
import datetime
import argparse

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tag', dest='version', type=str, required=True, help='tag for GitHub')
    parser.add_argument('--platform', dest='platform', type=str, required=True, help='platform for repository.json')
    parser.add_argument('--zipfile', dest='zipfile', type=str, required=True, help='location of the asset on disk')
    args = parser.parse_args()

    zipfile = os.path.basename(args.zipfile)

    repo = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'repository.json'))
    data = None
    with open(repo, 'r') as infile:
        data = json.loads(infile.read())
    if not data:
        return 1
    releases = data['packages'][0]['releases']
    updated = False
    for release in releases:
        updated = maybe_update_release(release, args.version, args.platform, zipfile)
        if updated:
            break

    if updated:
        print('Updated release %s for %s' % (args.version, args.platform))
    else:
        print('Creating new release %s for %s' % (args.version, args.platform))
        releases.append(create_new_release(args.version, args.platform, zipfile))

    with open(repo, 'w') as outfile:
        json.dump(data, outfile, indent=2, check_circular=False)
    return 0

def maybe_update_release(release, version, platform, zipfile):
    if release['version'] == version and release['platforms'][0] == platform:
        # update the release
        release['date'] = create_date()
        release['url'] = create_url(version, zipfile)
        return True
    else:
        return False

def create_new_release(version, platform, zipfile):
    release = { 
        'version': version, 
        'platforms': [platform], 
        'date': create_date(), 
        'url': create_url(version, zipfile), 
        'sublime_text': '>=3000' }
    return release

def create_url(version, zipfile):
    return 'https://github.com/rwols/Clara/releases/download/%s/%s' % (version, zipfile)

def create_date():
    now = datetime.datetime.utcnow()
    return now.strftime('%Y-%m-%d %H:%M:%S')


if __name__ == '__main__':
    exit(main())
