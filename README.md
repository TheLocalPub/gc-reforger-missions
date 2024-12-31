<p align="center">
    <img src="https://content.globalconflicts.net/ArtAssets/logo_black_text.png" width="1024">
</p>
<p align="center">
    <a href="https://github.com/Global-Conflicts-ArmA/gc-reforger-missions/releases/latest">
        <img src="https://img.shields.io/badge/Version-1.0.2-blue.svg" alt="Global Conflicts Reforger mission repo version">
</p>

# gc-reforger-missions
[GC Missions](https://reforger.armaplatform.com/workshop/62D55563549DA97A-GlobalConflictsMissions) is a repository of Arma Reforger missions created by and for the Global Conflicts community.

# Guidelines

**Content guidelines:**
- World file structure convention: GlobalConflictsMissions/worlds/AuthorName/MissionName/MissionName.ent
- Custom prefabs which are only meant to be used by this particular mission should be saved in worlds/AuthorName/MissionName/Prefabs/prefab.et, so together with the world
- World naming convention: Mission **world** names do not include spaces, type or player count, also keep them in **PascalCase** like e. g. "RoutePierre".
- Header naming convention: The name of the mission header config file matches the name of the world file, but the mission name specified **in** the header follows the "TYPE (MIN-MAX) MissionName" format, so for example "COOP (5-13) Gull Bay Guns".
- Prefabs that are useful to other mission makers can stay in the regular Prefabs folder, however GC Missions might not always be the right place for them. For example, the place for publicly accessible characters/groups is GC Units.
- Don't add new dependencies with are not already part of GC Core without asking. The exception would be if you know this mod has been greenlit, but GC Core is yet to be updated.
- Mission authors should never create overrides, whether to scripts or to prefabs, since they affect everything on the server.
- When changing existing content that was not created by you, please discuss it with the author first.

**Contribution guidelines:**
- Mission authors fork the GC Missions repo, and set it up on their machine (e. g. using GitHub Desktop). It is recommended to keep your fork somewhat synced with the parent repo while working on missions.
- If you are working on multiple missions in parallel, it is highly recommended to commit to branches of your fork, not everything to master. This is so you can create a PR for one branch/mission while other branches/missions are still WIP.
- When the mission is done, go to your forked repositiory and create a pull request (for that branch), describing the changes according to the PR template. The Mission Review Team may ask you to make some changes before it is accepted.
- In the future, there will be a more detailed guide.

**Review guidelines:**
- When a PR is made, Mission Review Team members first take a syntactical look at the changes, evaluating if e. g. folder structure is correct.
- Afterwards, they go to the fork/branch in question, and clone it to their machine. They test the mission locally according to testing guidelines.
- If there are any issues, they inform the author of the pull request, and repeat the process after the PR has been updated.
- If there were no issues, approve and merge the pull request. Also add any new missions to the spreadsheet.

**Testing guidelines:**
- Open the Arma Reforger Tools, adding the cloned repo to your projects.
- Examine new resources like prefabs, but also check if the header is configured correctly etc.
- Check if the worlds general setup is correct. Missing entities like e. g. PerceptionManager? Unconfigured / wrongly configured things, like NavmeshWorldComponents?
- Have a look at all framework related entities, understand the setup in terms of logic and look for potential mistakes.
- If it's a new mission maker, take a brief look at the mission concept - are there any glaring concept issues?
- Finally run the world using the Dedicated Server Tool to check for JIP errors. You do not have to test mission logic ingame as long as it uses the standard framework things for that.
- Custom solutions like new scripts or unconventional features should be tested ingame, including in MP using the DST.

**Publishing guidelines:**
- Mission authors do not publish the mod themselves, they create a pull request and let the Reforger MRT handle the rest. MRT members also follow this convention to keep things organized.
- MRT members may not be able to publish from their working directory, since it might include unfinished things that are not meant to be included yet. To avoid issues, only the current content of the main repo should ever be published. In the future, perhaps we can automate this, but for now, independently cloning the repo to publish would be one alternative.
