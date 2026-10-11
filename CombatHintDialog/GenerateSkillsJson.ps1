param(
    [string]$CreatureScript,
    [string]$OutputPath,
    [string]$GameRoot
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not $GameRoot) {
    $GameRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}
if (-not $CreatureScript) {
    $CreatureScript = Join-Path $GameRoot 'Mods\Heroes Enchanted 1.0\Data\s\Z Rebalancing Creatures.erm'
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $GameRoot 'Mods\Heroes Enchanted 1.0\Lang\Skills.json'
}

if (-not (Test-Path -LiteralPath $CreatureScript -PathType Leaf)) {
    throw "Creature description script not found: $CreatureScript"
}

$descriptions = @{
    '3-Hex Attack' = 'Attacks the chosen target and creatures standing on the two hexes immediately behind it.'
    'Abyssal Fury' = 'May enter an abyssal fury after attacking, gaining the combat bonuses defined by the ability.'
    'Accurate Shoot' = 'Its shot can strike several creatures when they stand in the projectile path.'
    'Acid Attack' = 'Its attack may cover the target in acid and weaken it.'
    'Acid Breath' = 'Its breath damages the target and may corrode creatures behind it.'
    'Age Attack' = 'Its attack may age the target, greatly reducing the target stack''s health.'
    'Agony Shoot' = 'Its shot may poison the target with a painful, lingering effect.'
    'Air Enchantment' = 'May enchant itself with Air Magic during combat.'
    'Air Shield' = 'May protect itself with Air Shield during combat.'
    'Army of Darkness' = 'Raises additional creatures by consuming the bodies of fallen enemies.'
    'Assimilation' = 'Can absorb fallen creatures to strengthen or restore its own stack.'
    'Attack All Around' = 'Attacks every enemy standing on an adjacent hex.'
    'Attack with Ice Bolt' = 'Its attack is accompanied by an Ice Bolt spell.'
    'Aura of Magic Res' = 'Grants nearby allied creatures additional magic resistance.'
    'Aura of Magic Resistance' = 'Grants nearby allied creatures additional magic resistance.'
    'Barbarian Rage' = 'May enter a barbarian rage and gain the combat bonuses defined by the ability.'
    'Behemoth Rage' = 'May unleash a behemoth rage and gain powerful combat bonuses.'
    'Beholder Shoot' = 'Uses a magical beholder ray as its ranged attack.'
    'Berserker Attack' = 'Its attack may drive the target Berserk.'
    'Berserker Rage' = 'May enter a berserker rage and gain the combat bonuses defined by the ability.'
    'Binding Attack' = 'Its attack may bind the target and prevent it from moving.'
    'Bless' = 'May cast Bless during combat.'
    'Bless of Faith' = 'May receive Bless of Faith, improving its combat performance.'
    'Blessed Shooter' = 'May become a Blessed Shooter and deal improved ranged damage.'
    'Blessing of the Forest' = 'Blesses allied creatures of the forest with beneficial combat effects.'
    'Blessing of the True Believer' = 'Blesses allied true believers with beneficial combat effects.'
    'Blind, Petrification and Death Stare Immunity' = 'Is immune to Blind, Petrification, and Death Stare effects.'
    'Blinding Attack' = 'Its attack may Blind the target.'
    'Blinding Shoot' = 'Its shot may Blind the target.'
    'Block Physical Damage' = 'May completely block damage from a physical attack.'
    'Body Looting' = 'May loot the bodies of slain enemies after combat actions.'
    'Breath' = 'Its breath attack also strikes the creature immediately behind the target.'
    'Breath and Binding Attack' = 'Its breath strikes through the target and may bind affected creatures.'
    'Cast Expert Berserker When Hit' = 'May cast Expert Berserk on the attacker when this stack is hit.'
    'Chaotic Attack' = 'Its attack may inflict one of several chaotic effects.'
    'Charge' = 'Deals additional melee damage for every hex crossed before the attack.'
    'Charge Immunity' = 'Does not suffer additional damage from enemy Charge abilities.'
    'Clone' = 'May create a temporary Clone of its own stack.'
    'Corrosive Breath' = 'Its breath may corrode the defenses of affected creatures.'
    'Crippling Attack' = 'Its attack may reduce the target''s Attack, Defense, and Speed.'
    'Crippling Shoot' = 'Its shot may reduce the target''s Attack, Defense, and Speed.'
    'Cure' = 'May cast Cure during combat.'
    'Curse of the Sorceress' = 'May afflict enemy creatures with the Curse of the Sorceress.'
    'Cursed Attack' = 'Its attack may Curse the target.'
    'Deadly Touch' = 'Its touch may instantly slay part of the target stack.'
    'Death Blow' = 'Has the stated chance to deal a devastating double-damage blow.'
    'Death Cloud Shoot' = 'Its shot explodes in a death cloud that also damages nearby living creatures.'
    'Death Stare' = 'After attacking, may slay additional creatures with its Death Stare.'
    'Defensive Clone' = 'May create a Clone when defending itself.'
    'Defensive Formation' = 'Can assume a defensive formation, trading offensive strength for greater Defense.'
    'Destroy Armor Attack' = 'Its attack permanently reduces the target''s Defense for the battle.'
    'Devour' = 'May devour a fallen or weakened creature to restore its own stack.'
    'Diabolic Fury' = 'May enter a diabolic fury and gain the combat bonuses defined by the ability.'
    'Disease Attack' = 'Its attack may Disease the target, reducing Attack and Defense.'
    'Dispel Buff Attack' = 'Its attack removes beneficial spells from the target.'
    'Double Attack' = 'Attacks twice each time it performs a normal attack.'
    'Double Shoot' = 'Shoots twice each time it performs a normal ranged attack.'
    'Dragonic Fury' = 'May enter a draconic fury and gain powerful combat bonuses.'
    'Drain Life' = 'Restores lost hit points according to the damage dealt to living enemies.'
    'Drain Spell Points' = 'Its attack drains spell points from the opposing hero.'
    'Ethereal' = 'Its immaterial form can cause physical attacks to pass harmlessly through it.'
    'Executioner' = 'Deals especially deadly damage to badly wounded enemy stacks.'
    'Expert Prayer' = 'May cast Expert Prayer during combat.'
    'Fear' = 'May terrify an enemy stack and cause it to lose its action.'
    'Fearless' = 'Is immune to Fear.'
    'Ferocity' = 'Continues attacking while its blows keep killing creatures, up to the ability limit.'
    'Fighting Ritual' = 'Performs a fighting ritual that grants combat bonuses.'
    'Fire Power' = 'Empowers itself with Fire Magic during combat.'
    'Fire Wall Attack' = 'Its attack may create a Fire Wall on the battlefield.'
    'Fireball Shoot' = 'Its projectile explodes like a Fireball, damaging creatures around the target hex.'
    'Fleash Eater' = 'Consumes enemy remains to restore hit points and revive creatures in its stack.'
    'Fungal War Potion' = 'May use a fungal war potion to gain combat bonuses.'
    'Glory of the Knight' = 'May receive Glory of the Knight, improving its combat abilities.'
    'Guard Break Blow' = 'Its blow is especially effective against guarding and defensive enemies.'
    'Haste' = 'May cast Haste during combat.'
    'Hates War Machines' = 'Deals increased damage to war machines.'
    'Hex Reaction' = 'May immediately react when a hostile hex or curse effect is used.'
    'Hex Shoot' = 'Its shot may inflict a hostile hex on the target.'
    'Hypnotize Attack' = 'Its attack may place the target under Hypnotize.'
    'Ice and Steel Enchantment' = 'Enchants its weapons with ice and steel effects.'
    'Ice Bolt Attack' = 'Its attack may strike the target with Ice Bolt.'
    'Ice Bolt Shoot' = 'Its shot may strike the target with Ice Bolt.'
    'Implosion Attack' = 'Its attack may trigger an Implosion on the target.'
    'Incapacitating Shoot' = 'Its shot may greatly reduce the target''s Attack, Defense, and Speed.'
    'Infernal Fever Attack' = 'Its attack may infect the target with Infernal Fever.'
    'Infernal Fury' = 'May enter an infernal fury and gain the combat bonuses defined by the ability.'
    'Inferno Attack' = 'Its attack may trigger an Inferno around the target.'
    'Invoke Elementals' = 'May call elemental creatures onto the battlefield.'
    'Jackpot' = 'May trigger a powerful random beneficial result.'
    'Kill Frenzy' = 'After its first eligible kill each turn, immediately gains another action.'
    'Killer Instinct' = 'May awaken its killer instinct and gain offensive combat bonuses.'
    'Lightning Attack' = 'Its attack may strike the target with lightning.'
    'Lightning Strike' = 'May call down a lightning strike on its target.'
    'Magic Arrow Attack' = 'Its attack may strike the target with Magic Arrow.'
    'Magic Mirror' = 'May reflect hostile spells back at their caster.'
    'Magic Res' = 'Has a chance to resist hostile spells completely.'
    'Magic Resistance' = 'Has a chance to resist hostile spells completely.'
    'Magic of the Fairy' = 'Uses fairy magic to cast beneficial or hostile spells.'
    'Magic of the Sprite' = 'Uses sprite magic to cast beneficial or hostile spells.'
    'Magical Shoot' = 'Its ranged attack may cast a random offensive spell on the target.'
    'Manticore Poison Attack' = 'Its attack may poison the target with manticore venom.'
    'Mass Counterstrike' = 'May cast Mass Counterstrike on allied creatures.'
    'Mass Fighting Ritual' = 'Performs a fighting ritual that benefits allied creatures.'
    'Mass Haste' = 'May cast Mass Haste on allied creatures.'
    'Mass Protection From Air' = 'May cast Mass Protection from Air on allied creatures.'
    'Mastermind Shoot' = 'Its shot may control or confuse the target''s mind.'
    'Might of the Nature' = 'Draws upon nature to improve its combat abilities.'
    'Mind Blast Shoot' = 'Its shot releases a mental blast against the target.'
    'Misfortune Attack' = 'Its attack may cast Misfortune on the target.'
    'Monstrous Rage' = 'May enter a monstrous rage and gain very powerful combat bonuses.'
    'No Distance Penalty' = 'Deals full ranged damage at any distance.'
    'No Retaliation' = 'Enemies cannot retaliate against its normal attacks.'
    'No Walls Penalty' = 'Deals full ranged damage through castle walls.'
    'Paralyzing Attack' = 'Its attack may Paralyze the target.'
    'Paralyzing Shoot' = 'Its shot may Paralyze the target.'
    'Parry' = 'Has the stated chance to parry an attack and reduce its physical damage by half.'
    'Petrifying Attack' = 'Its attack may Petrify the target.'
    'Piercing Attack' = 'Ignores part of the target''s Defense when calculating damage.'
    'Poisonous Attack' = 'Its attack may Poison the target.'
    'Poisonous Breath' = 'Its breath may Poison every creature it strikes.'
    'Poisonous Shoot' = 'Its shot may Poison the target.'
    'Polymorph Attack' = 'After dealing damage, may permanently transform the target into another creature type.'
    'Polymorphic Shoot' = 'Its ranged attack may permanently transform the target into another creature type.'
    'Polymorphic Strike' = 'Its melee strike may permanently transform the target into another creature type.'
    'Powerful Hex Shoot' = 'Its shot may inflict a powerful hostile hex on the target.'
    'Powerful Magical Shoot' = 'Its ranged attack may cast a powerful offensive spell on the target.'
    'Powerful Random Offensive Spellcaster' = 'Can cast a powerful random offensive spell during combat.'
    'Prayer' = 'May cast Prayer during combat.'
    'Preventive Retaliation' = 'Retaliates before the enemy completes its melee attack.'
    'Protection of the Archmage' = 'May receive the Archmage''s magical protection.'
    'Protection of the Grandmaster' = 'May receive the Grandmaster''s magical protection.'
    'Protection of the Master' = 'May receive the Master''s magical protection.'
    'Pure Light Breath' = 'Its breath releases pure light through the target and affects creatures behind it.'
    'Random Mass Buff Every Turn' = 'At the start of each turn, casts a random beneficial mass spell.'
    'Random Mass Debuff Every Turn' = 'At the start of each turn, casts a random hostile mass spell.'
    'Random Mass Spells Every 3 Turns' = 'Every third turn, casts a random mass spell.'
    'Random Offensive Spellcaster' = 'Can cast a random offensive spell during combat.'
    'Ranged Retaliation' = 'Can retaliate against ranged attacks with a ranged attack of its own.'
    'Recycling' = 'Reuses remains or summoned matter to restore resources or creatures.'
    'Repair Construct 1' = 'Can repair allied Constructs, restoring lost hit points and creatures.'
    'Rest in Peace' = 'Creatures slain by this stack cannot be resurrected or raised.'
    'Resurrection' = 'May restore fallen living creatures to its stack.'
    'Revenge' = 'Deals increased damage as its own stack becomes wounded.'
    'Sea Hex Shoot' = 'Its shot may inflict a hostile sea hex on the target.'
    'Shattering Spree' = 'After its first eligible kill each turn, immediately gains another action.'
    'Shield' = 'May cast Shield during combat.'
    'Shield Block' = 'Has the stated chance to block all physical damage from an attack.'
    'Shield of the Empire' = 'May receive Shield of the Empire, reducing incoming damage.'
    'Slayer' = 'May cast Slayer during combat.'
    'Slow Shoot' = 'Its shot may cast Slow on the target.'
    'Sorceress Shoot' = 'Its shot may inflict one of the Sorceress''s hostile spells.'
    'Sorrow Attack' = 'Its attack may cast Sorrow on the target.'
    'Spell Damage Reduction' = 'Reduces damage received from hostile spells.'
    'Steal Artifact Attack' = 'Its attack may steal an artifact from the opposing hero.'
    'Steal Money and Artifact Attack' = 'Its attack may steal gold and an artifact from the opposing hero.'
    'Steal Money Attack' = 'Its attack may steal gold from the opposing hero.'
    'Strike and Return' = 'Returns to its starting hex after completing a melee attack.'
    'Strong Diabolic Fury' = 'May enter a powerful diabolic fury and gain major combat bonuses.'
    'Stunning Attack' = 'Its attack may Stun the target, making it lose its action and retaliations.'
    'Stunning Strike' = 'Its strike may Stun the target, making it lose its action and retaliations.'
    'Summon Fire Elementals' = 'May summon Fire Elementals onto the battlefield.'
    'Supreme Magical Shoot' = 'Its ranged attack may cast a supreme offensive spell on the target.'
    'Thirst for Blood' = 'May enter a bloodthirsty state and gain offensive combat bonuses.'
    'Two Retaliations' = 'Can retaliate twice during each combat round.'
    'Undead Nemesis' = 'After its first eligible kill of an Undead enemy each turn, immediately gains another action.'
    'Underworld of Magic' = 'Draws random magic from the underworld during combat.'
    'Unlimited Retaliations' = 'Can retaliate an unlimited number of times each combat round.'
    'Wall of Shields Formation' = 'Can form a wall of shields, trading offensive strength for greatly increased Defense.'
    'Weak Drain Life' = 'Restores a reduced amount of lost hit points according to damage dealt to living enemies.'
    'Weakness Attack' = 'Its attack may cast Weakness on the target.'
    'Wild Lightning Attack' = 'Its attack may release unpredictable lightning against the target.'
    'Wild Spells Attack' = 'Its attack may trigger one of several unpredictable spells.'
    'Will Spells Immunity' = 'Is immune to spells that control or alter a creature''s will.'
    'World of Magic' = 'Draws a random spell from the wider world of magic during combat.'
    'Wrath of Vengeance' = 'May enter a vengeful wrath and gain defensive and offensive combat bonuses.'
}

function Get-GenericDescription([string]$Name) {
    if ($descriptions.ContainsKey($Name)) {
        return $descriptions[$Name]
    }

    if ($Name -match '^Strongly Hates (.+)$') {
        return "Deals greatly increased damage to $($Matches[1])."
    }
    if ($Name -match '^Ignore (\d+)% (?:of )?Enemy (Attack|Defense) Skill$') {
        return "Ignores $($Matches[1])% of the enemy's $($Matches[2]) Skill when calculating damage."
    }
    if ($Name -match '^-(\d+)% Magic Damage$') {
        return "Reduces damage received from hostile spells by $($Matches[1])%."
    }
    if ($Name -match '^Cast (.+)$') {
        return "Can cast $($Matches[1]) during combat."
    }
    if ($Name -match '^Raise (.+?)(?: 1)?$') {
        return "Can raise $($Matches[1]) from fallen creatures."
    }
    if ($Name -match '^(?:All )?Slain (.+)$') {
        return 'Transforms creatures slain by this stack as stated by the ability.'
    }
    if ($Name -match '^(.+) Affinity$') {
        return "Has an affinity with $($Matches[1]) magic and its related spells."
    }
    if ($Name -match '^Level (.+) Hostile Spells Immunity$') {
        return "Is immune to hostile spells of levels $($Matches[1])."
    }
    if ($Name -match '^Level (.+) Spells Immunity$') {
        return "Is immune to spells of levels $($Matches[1])."
    }
    if ($Name -match '^(.+) Immunity$') {
        return "Is immune to $($Matches[1])."
    }

    return 'Description to be added'
}

function Get-SkillId([string]$Name) {
    $id = $Name.ToLowerInvariant()
    $id = $id -replace '^3-', 'three_'
    $id = $id -replace '^-', 'minus_'
    $id = $id -replace '&', ' and '
    $id = $id -replace '[^a-z0-9]+', '_'
    return $id.Trim('_')
}

function Add-Phrases([System.Collections.Generic.List[string]]$Target, [string]$Body) {
    $Body = [regex]::Replace($Body, '\{~[0-9A-Fa-f]{6}\}', '')
    foreach ($piece in ($Body -split '[\.\r\n;]+')) {
        $name = $piece.Trim()
        if (-not $name) {
            continue
        }

        $name = $name -replace '^\d+%\s*', ''
        $name = $name -replace '^Cast\s+\d+\s+', 'Cast '
        $name = $name -replace '\s+', ' '
        $name = $name.Trim()
        if ($name) {
            $Target.Add($name)
        }
    }
}

$phrases = [System.Collections.Generic.List[string]]::new()
$customPattern = '\{~[0-9A-Fa-f]{6}\}(?<body>.*?)\{~\}'

foreach ($line in Get-Content -LiteralPath $CreatureScript) {
    if ($line -notmatch 'SN:H\^monname\^/\d+/2/') {
        continue
    }

    $description = ($line -split '/2/', 2)[1]
    foreach ($match in [regex]::Matches($description, $customPattern)) {
        Add-Phrases $phrases $match.Groups['body'].Value
    }

    $withoutCustom = [regex]::Replace($description, $customPattern, '')
    foreach ($match in [regex]::Matches($withoutCustom, '\{(?<body>[^{}]+)\}')) {
        Add-Phrases $phrases $match.Groups['body'].Value
    }
}

$names = @($phrases | Sort-Object -Unique)
$skills = [ordered]@{}
$ids = [System.Collections.Generic.List[string]]::new()

foreach ($name in $names) {
    $id = Get-SkillId $name
    if (-not $id) {
        throw "Could not generate an id for skill '$name'."
    }
    if ($skills.Contains($id)) {
        throw "Duplicate generated id '$id' for '$name' and '$($skills[$id].name)'."
    }

    $ids.Add($id)
    $skills[$id] = [ordered]@{
        name = $name
        description = Get-GenericDescription $name
    }
}

$json = [ordered]@{
    combatHintDialog = [ordered]@{
        title = 'Creature Abilities'
        noSkills = 'No highlighted ability descriptions are available for this creature.'
        skillIds = ($ids -join '|')
        skills = $skills
    }
}

$parent = Split-Path -Parent $OutputPath
if (-not (Test-Path -LiteralPath $parent -PathType Container)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}

$text = $json | ConvertTo-Json -Depth 8
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($OutputPath, $text + "`r`n", $utf8NoBom)

[pscustomobject]@{
    OutputPath = $OutputPath
    Skills = $skills.Count
    PlaceholderDescriptions = @($skills.Values | Where-Object { $_.description -eq 'Description to be added' }).Count
}
